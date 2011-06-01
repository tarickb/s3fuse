#include "logging.hh"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <pugixml/pugixml.hpp>

#include "file_transfer.hh"
#include "fs.hh"
#include "mutexes.hh"
#include "request.hh"
#include "util.hh"

using namespace boost;
using namespace pugi;
using namespace std;

using namespace s3;

// TODO: try/catch everywhere! (or, get rid of exceptions)
// TODO: support multiple buckets (with a different set of credentials each)
// TODO: rewrite using libxml2?
// TODO: rename .cc to .. what?
// TODO: set up for autoconf

#define ASSERT_NO_TRAILING_SLASH(str) do { if ((str)[(str).size() - 1] == '/') return -EINVAL; } while (0)

namespace
{
  const string SYMLINK_PREFIX = "SYMLINK:";

  const xpath_query PREFIX_QUERY("/ListBucketResult/CommonPrefixes/Prefix");
  const xpath_query KEY_QUERY("/ListBucketResult/Contents");
}

fs::fs()
  : _tp_fg(thread_pool::create("fs-fg")),
    _tp_bg(thread_pool::create("fs-bg")),
    _mutexes(new mutexes()),
    _object_cache(_tp_fg, _mutexes, file_transfer::ptr(new file_transfer(_tp_fg, _tp_bg)))
{
}

fs::~fs()
{
  _tp_fg->terminate();
  _tp_bg->terminate();
}

int fs::remove_object(const request::ptr &req, const string &url)
{
  req->init(HTTP_DELETE);
  req->set_url(url);

  req->run();

  return (req->get_response_code() == 204) ? 0 : -EIO;
}

typedef shared_ptr<string> string_ptr;

struct rename_operation
{
  string_ptr old_name;
  async_handle handle;
};

int fs::rename_children(const request::ptr &req, const string &_from, const string &_to)
{
  size_t from_len;
  string marker = "";
  bool truncated = true;
  string from, to;
  list<rename_operation> pending_renames, pending_deletes;

  if (_from.empty())
    return -EINVAL;

  from = _from + "/";
  to = _to + "/";
  from_len = from.size();

  req->init(HTTP_GET);

  while (truncated) {
    xml_document doc;
    xml_parse_result res;
    xpath_node_set prefixes, keys;

    req->set_url(object::get_bucket_url(), string("prefix=") + util::url_encode(from) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != 200)
      return -EIO;

    res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

    if (res.status != status_ok) {
      S3_DEBUG("fs::rename_children", "failed to parse response: %s\n", res.description());
      return -EIO;
    }

    truncated = (strcmp(doc.document_element().child_value("IsTruncated"), "true") == 0);
    keys = KEY_QUERY.evaluate_node_set(doc);

    if (truncated)
      marker = doc.document_element().child_value("NextMarker");

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      rename_operation oper;
      const char *full_path_cs = itor->node().child_value("Key");
      const char *relative_path_cs = full_path_cs + from_len;
      string new_name = to + relative_path_cs;

      oper.old_name.reset(new string(full_path_cs));
      oper.handle = _tp_bg->post(bind(&fs::copy_file, this, _1, *oper.old_name, new_name));

      pending_renames.push_back(oper);

      S3_DEBUG("fs::rename_children", "[%s] -> [%s]\n", full_path_cs, new_name.c_str());
    }
  }

  while (!pending_renames.empty()) {
    int r;
    rename_operation &oper = pending_renames.front();

    r = _tp_bg->wait(oper.handle);

    if (r)
      return r;

    oper.handle.reset();
    pending_deletes.push_back(oper);
    pending_renames.pop_front();
  }

  // specify OT_FILE because it doesn't transform the path
  for (list<rename_operation>::iterator itor = pending_deletes.begin(); itor != pending_deletes.end(); ++itor)
    itor->handle = _tp_bg->post(bind(&fs::remove_object, this, _1, object::build_url(*itor->old_name, OT_FILE)));

  while (!pending_deletes.empty()) {
    int r;
    const rename_operation &oper = pending_deletes.front();

    r = _tp_bg->wait(oper.handle);
    pending_deletes.pop_front();

    if (r)
      return r;
  }

  return 0;
}

bool fs::is_directory_empty(const request::ptr &req, const string &path)
{
  xml_document doc;
  xml_parse_result res;
  xpath_node_set keys;

  ASSERT_NO_TRAILING_SLASH(path);

  // root directory may be empty, but we won't allow its removal
  if (path.empty())
    return false;

  req->init(HTTP_GET);

  // set max-keys to two because GET will always return the path we request
  // note the trailing slash on path
  req->set_url(object::get_bucket_url(), string("prefix=") + util::url_encode(path) + "/&max-keys=2");
  req->run();

  // if the request fails, assume the directory's not empty
  if (req->get_response_code() != 200)
    return false;

  res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

  if (res.status != status_ok) {
    S3_DEBUG("fs::is_directory_empty", "failed to parse response: %s\n", res.description());
    return false;
  }

  keys = KEY_QUERY.evaluate_node_set(doc);

  return (keys.size() == 1);
}

int fs::__prefill_stats(const request::ptr &req, const string &path, int hints)
{
  _object_cache.get(req, path, hints);

  return 0;
}

int fs::__get_stats(const request::ptr &req, const string &path, struct stat *s, int hints)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path, hints);

  if (!obj)
    return -ENOENT;

  if (s)
    obj->copy_stat(s);

  return 0;
}

int fs::__rename_object(const request::ptr &req, const string &from, const string &to)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(from);
  ASSERT_NO_TRAILING_SLASH(to);

  obj = _object_cache.get(req, from);

  if (!obj)
    return -ENOENT;

  if (_object_cache.get(req, to))
    return -EEXIST;

  if (obj->get_type() == OT_DIRECTORY)
    return rename_children(req, from, to);
  else {
    int r = copy_file(req, from, to);

    if (r)
      return r;

    _object_cache.remove(from);
    return remove_object(req, obj->get_url());
  }
}

int fs::copy_file(const request::ptr &req, const string &from, const string &to)
{
  req->init(HTTP_PUT);
  req->set_url(object::build_url(to, OT_FILE));
  req->set_header("x-amz-copy-source", object::build_url(from, OT_FILE));
  req->set_header("x-amz-metadata-directive", "COPY");

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}

int fs::__change_metadata(const request::ptr &req, const string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path);

  if (!obj)
    return -ENOENT;

  if (mode != mode_t(-1))
    obj->set_mode(mode);

  if (uid != uid_t(-1))
    obj->set_uid(uid);

  if (gid != gid_t(-1))
    obj->set_gid(gid);

  if (mtime != time_t(-1))
    obj->set_mtime(mtime);

  return obj->commit_metadata(req);
}

int fs::__read_directory(const request::ptr &req, const string &_path, fuse_fill_dir_t filler, void *buf)
{
  size_t path_len;
  string marker = "";
  bool truncated = true;
  string path;

  ASSERT_NO_TRAILING_SLASH(_path);

  if (!_path.empty())
    path = _path + "/";

  path_len = path.size();

  req->init(HTTP_GET);

  while (truncated) {
    xml_document doc;
    xml_parse_result res;
    xpath_node_set prefixes, keys;

    req->set_url(object::get_bucket_url(), string("delimiter=/&prefix=") + util::url_encode(path) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != 200)
      return -EIO;

    res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

    if (res.status != status_ok) {
      S3_DEBUG("fs::__read_directory", "failed to parse response: %s\n", res.description());
      return -EIO;
    }

    truncated = (strcmp(doc.document_element().child_value("IsTruncated"), "true") == 0);
    prefixes = PREFIX_QUERY.evaluate_node_set(doc);
    keys = KEY_QUERY.evaluate_node_set(doc);

    if (truncated)
      marker = doc.document_element().child_value("NextMarker");

    for (xpath_node_set::const_iterator itor = prefixes.begin(); itor != prefixes.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value();
      const char *relative_path_cs = full_path_cs + path_len;
      string full_path, relative_path;

      // strip trailing slash
      full_path.assign(full_path_cs, strlen(full_path_cs) - 1);
      relative_path.assign(relative_path_cs, strlen(relative_path_cs) - 1);

      _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_DIR));
      filler(buf, relative_path.c_str(), NULL, 0);
    }

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value("Key");

      if (strcmp(path.data(), full_path_cs) != 0) {
        string relative_path(full_path_cs + path_len);
        string full_path(full_path_cs);

        _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_FILE));
        filler(buf, relative_path.c_str(), NULL, 0);
      }
    }
  }

  return 0;
}

int fs::__create_object(const request::ptr &req, const string &path, object_type type, mode_t mode, const string &symlink_target)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  if (_object_cache.get(req, path)) {
    S3_DEBUG("fs::__create_object", "attempt to overwrite object at path %s.\n", path.c_str());
    return -EEXIST;
  }

  obj.reset(new object(_mutexes, path, type));
  obj->set_mode(mode);

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);

  if (type == OT_SYMLINK)
    req->set_input_data(SYMLINK_PREFIX + symlink_target);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}

int fs::__remove_object(const request::ptr &req, const string &path)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path, HINT_NONE);

  if (!obj)
    return -ENOENT;

  if (obj->get_type() == OT_DIRECTORY && !is_directory_empty(req, obj->get_path()))
    return -ENOTEMPTY;

  _object_cache.remove(path);
  return remove_object(req, obj->get_url());
}

int fs::__read_symlink(const request::ptr &req, const string &path, string *target)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path);

  if (!obj)
    return -ENOENT;

  if (obj->get_type() != OT_SYMLINK)
    return -EINVAL;

  req->init(HTTP_GET);
  req->set_url(obj->get_url());

  req->run();
  
  if (req->get_response_code() != 200)
    return -EIO;

  if (req->get_response_data().substr(0, SYMLINK_PREFIX.size()) != SYMLINK_PREFIX)
    return -EINVAL;

  *target = req->get_response_data().substr(SYMLINK_PREFIX.size());
  return 0;
}

int fs::__set_attr(const request::ptr &req, const string &path, const string &name, const string &value, int flags)
{
  object::ptr obj;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path);

  if (!obj)
    return -ENOENT;

  r = obj->set_metadata(name, value, flags);

  if (r)
    return r;

  return obj->commit_metadata(req);
}

int fs::__remove_attr(const request::ptr &req, const string &path, const string &name)
{
  object::ptr obj;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path);

  if (!obj)
    return -ENOENT;

  r = obj->remove_metadata(name);

  if (r)
    return r;

  return obj->commit_metadata(req);
}
