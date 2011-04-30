#include "logging.hh"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "fs.hh"
#include "request.hh"
#include "util.hh"

using namespace boost;
using namespace pugi;
using namespace std;

using namespace s3;

// TODO: try/catch everywhere!
// TODO: check error codes after run()

#define ASSERT_NO_TRAILING_SLASH(str) do { if ((str)[(str).size() - 1] == '/') return -EINVAL; } while (0)

namespace
{
  const string SYMLINK_PREFIX = "SYMLINK:";
}

fs::fs()
  : _prefix_query("/ListBucketResult/CommonPrefixes/Prefix"),
    _key_query("/ListBucketResult/Contents"),
    _tp_fg(new thread_pool("fs-fg")),
    _tp_bg(new thread_pool("fs-bg")),
    _object_cache(_tp_fg),
    _open_file_cache(_tp_fg)
{
}

int fs::remove_object(const request::ptr &req, const object::ptr &obj)
{
  req->init(HTTP_DELETE);
  req->set_url(obj->get_url());

  // TODO: obviously (OBVIOUSLY!) this should check if obj is a directory

  req->run();

  _object_cache.remove(obj->get_path());

  return (req->get_response_code() == 204) ? 0 : -EIO;
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

int fs::__rename_object(const request::ptr &req, const std::string &from, const std::string &to)
{
  object::ptr obj;
  string to_url;

  ASSERT_NO_TRAILING_SLASH(from);
  ASSERT_NO_TRAILING_SLASH(to);

  obj = _object_cache.get(req, from);

  if (!obj)
    return -ENOENT;

  // TODO: check if directory has children!

  if (_object_cache.get(req, to))
    return -EEXIST;

  to_url = object::build_url(to, obj->get_type());

  req->init(HTTP_PUT);
  req->set_url(to_url);
  req->set_header("Content-Type", obj->get_content_type());
  req->set_header("x-amz-copy-source", obj->get_url());
  req->set_header("x-amz-copy-source-if-match", obj->get_etag());
  req->set_header("x-amz-metadata-directive", "COPY");

  req->run();

  if (req->get_response_code() != 200)
    return -EIO;

  return remove_object(req, obj);
}

int fs::__change_metadata(const request::ptr &req, const std::string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime)
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

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_header("x-amz-copy-source", obj->get_url());
  req->set_header("x-amz-copy-source-if-match", obj->get_etag());
  req->set_header("x-amz-metadata-directive", "REPLACE");
  req->set_meta_headers(obj);

  req->run();

  if (req->get_response_code() != 200) {
    S3_DEBUG("fs::change_metadata", "response: %s\n", req->get_response_data().c_str());
    return -EIO;
  }

  // TODO: do we need to remove the object from the cache?

  return 0;
}

int fs::__read_directory(const request::ptr &req, const std::string &_path, fuse_fill_dir_t filler, void *buf)
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

    res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

    truncated = (strcmp(doc.document_element().child_value("IsTruncated"), "true") == 0);
    prefixes = _prefix_query.evaluate_node_set(doc);
    keys = _key_query.evaluate_node_set(doc);

    if (truncated)
      marker = doc.document_element().child_value("NextMarker");

    for (xpath_node_set::const_iterator itor = prefixes.begin(); itor != prefixes.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value();
      const char *relative_path_cs = full_path_cs + path_len;
      string full_path, relative_path;

      // strip trailing slash
      full_path.assign(full_path_cs, strlen(full_path_cs) - 1);
      relative_path.assign(relative_path_cs, strlen(relative_path_cs) - 1);

      S3_DEBUG("fs::read_directory", "found common prefix [%s]\n", relative_path.c_str());

      _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_DIR));
      filler(buf, relative_path.c_str(), NULL, 0);
    }

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value("Key");

      if (strcmp(path.data(), full_path_cs) != 0) {
        string relative_path(full_path_cs + path_len);
        string full_path(full_path_cs);

        S3_DEBUG("fs::read_directory", "found key [%s]\n", relative_path.c_str());

        _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_FILE));
        filler(buf, relative_path.c_str(), NULL, 0);
      }
    }
  }

  return 0;
}

int fs::__create_object(const request::ptr &req, const std::string &path, object_type type, mode_t mode, const std::string &symlink_target)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  if (_object_cache.get(req, path)) {
    S3_DEBUG("fs::create_object", "attempt to overwrite object at path %s.\n", path.c_str());
    return -EEXIST;
  }

  obj.reset(new object(path));
  obj->set_defaults(type);
  obj->set_mode(mode);

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);

  if (type == OT_SYMLINK)
    req->set_input_data(SYMLINK_PREFIX + symlink_target);

  req->run();

  return 0;
}

int fs::__remove_object(const request::ptr &req, const std::string &path)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path, HINT_NONE);

  return obj ? remove_object(req, obj) : -ENOENT;
}

int fs::__read_symlink(const request::ptr &req, const std::string &path, std::string *target)
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
