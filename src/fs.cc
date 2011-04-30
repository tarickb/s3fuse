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

fs::fs()
  : _prefix_query("/ListBucketResult/CommonPrefixes/Prefix"),
    _key_query("/ListBucketResult/Contents"),
    _tp_fg(new thread_pool("fs-fg")),
    _tp_bg(new thread_pool("fs-bg")),
    _object_cache(_tp_fg),
    _open_file_cache(_tp_fg)
{
}

ASYNC_DEF(fs, prefill_stats, const string &path, int hints)
{
  _object_cache.get(req, path, hints);

  return 0;
}

ASYNC_DEF(fs, get_stats, const string &path, struct stat *s, int hints)
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

ASYNC_DEF(fs, rename_object, const std::string &from, const std::string &to)
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

ASYNC_DEF(fs, change_metadata, const std::string &path, mode_t mode, uid_t uid, gid_t gid)
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

  _object_cache.remove(path);

  return 0;
}

ASYNC_DEF(fs, read_directory, const std::string &_path, fuse_fill_dir_t filler, void *buf)
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

      ASYNC_CALL_NONBLOCK(_tp_bg, fs, prefill_stats, full_path, HINT_IS_DIR);
      filler(buf, relative_path.c_str(), NULL, 0);
    }

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value("Key");

      if (strcmp(path.data(), full_path_cs) != 0) {
        string relative_path(full_path_cs + path_len);
        string full_path(full_path_cs);

        S3_DEBUG("fs::read_directory", "found key [%s]\n", relative_path.c_str());

        ASYNC_CALL_NONBLOCK(_tp_bg, fs, prefill_stats, full_path, HINT_IS_FILE);
        filler(buf, relative_path.c_str(), NULL, 0);
      }
    }
  }

  return 0;
}

ASYNC_DEF(fs, create_object, const std::string &path, mode_t mode)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  if (_object_cache.get(req, path)) {
    S3_DEBUG("fs::create_object", "attempt to overwrite object at path %s.\n", path.c_str());
    return -EEXIST;
  }

  obj.reset(new object(path));
  obj->set_defaults(S_ISDIR(mode) ? OT_DIRECTORY : OT_FILE);

  // TODO: use parent?
  obj->set_mode(mode);

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);

  req->run();

  return 0;
}

/*
ASYNC_DEF(open, const std::string &path, uint64_t *context)
{
  string url;
  FILE *temp_file;
  handle_ptr handle;

  ASSERT_NO_TRAILING_SLASH(path);

  url = _bucket + "/" + util::url_encode(path);
  temp_file = tmpfile();

  if (!temp_file)
    return -errno;

  req->init(HTTP_GET);
  req->set_url(url);
  req->set_output_file(temp_file);

  req->run();

  if (req->get_response_code() != 200) {
    fclose(temp_file);
    return (req->get_response_code() == 404) ? -ENOENT : -EIO;
  }

  fflush(temp_file);

  handle.reset(new file_handle);

  handle->status = FS_NONE;
  handle->path = path;
  handle->etag = req->get_response_header("ETag");
  handle->content_type = req->get_response_header("Content-Type");
  handle->local_fd = temp_file;

  std::string pfx = "x-amz-meta-";

  // TODO: keep only one copy of this, in a shared_ptr, in request
  for (string_map::const_iterator itor = req->get_response_headers().begin(); itor != req->get_response_headers().end(); ++itor)
    if (itor->first.substr(0, pfx.size()) == pfx)
      handle->metadata[itor->first] = itor->second;

  {
    mutex::scoped_lock lock(_open_files_mutex);

    *context = _next_open_file_handle++;
    _open_files[*context] = handle;
  }

  S3_DEBUG("fs::open", "opened file %s with context %" PRIu64 ".\n", path.c_str(), *context);

  return 0;
}

ASYNC_DEF(flush, uint64_t context)
{
  mutex::scoped_lock lock(_open_files_mutex);
  handle_map::iterator itor = _open_files.find(context);
  handle_ptr handle;
  int r = 0;

  // TODO: this is very similar to close() below

  if (itor == _open_files.end())
    return -EINVAL;

  handle = itor->second;

  if (handle->status & FS_IN_USE)
    return -EBUSY;

  if (handle->status & FS_FLUSHING)
    return 0; // another thread is closing this file

  handle->status |= FS_FLUSHING;
  lock.unlock();

  if (handle->status & FS_DIRTY)
    r = flush(req, handle);

  lock.lock();
  handle->status &= ~FS_FLUSHING;

  if (!r)
    handle->status &= ~FS_DIRTY;

  return r;
}

ASYNC_DEF(close, uint64_t context)
{
  mutex::scoped_lock lock(_open_files_mutex);
  handle_map::iterator itor = _open_files.find(context);
  handle_ptr handle;
  int r = 0;

  if (itor == _open_files.end())
    return -EINVAL;

  // TODO: this code has been duplicated far too many times
  handle = itor->second;

  if (handle->status & FS_IN_USE || handle->status & FS_FLUSHING)
    return -EBUSY;

  handle->status |= FS_FLUSHING;
  lock.unlock();

  if (handle->status & FS_DIRTY)
    r = flush(req, handle);

  lock.lock();
  handle->status &= ~FS_FLUSHING;

  if (!r) {
    handle->status &= ~FS_DIRTY;
    _open_files.erase(itor);
  }

  _stats_cache.remove(handle->path);

  return r;
}

int fs::read(char *buffer, size_t size, off_t offset, uint64_t context)
{
  mutex::scoped_lock lock(_open_files_mutex);
  handle_map::iterator itor = _open_files.find(context);
  handle_ptr handle;
  int r;

  if (itor == _open_files.end())
    return -EINVAL;

  handle = itor->second;

  if (handle->status & FS_FLUSHING)
    return -EBUSY;

  handle->status |= FS_IN_USE;
  lock.unlock();

  r = pread(fileno(handle->local_fd), buffer, size, offset);

  lock.lock();
  handle->status &= ~FS_IN_USE;

  return r;
}

int fs::write(const char *buffer, size_t size, off_t offset, uint64_t context)
{
  mutex::scoped_lock lock(_open_files_mutex);
  handle_map::iterator itor = _open_files.find(context);
  handle_ptr handle;
  int r;

  if (itor == _open_files.end()) {
    S3_DEBUG("fs::write", "cannot find file with context %" PRIu64 ".\n", context);
    return -EINVAL;
  }

  handle = itor->second;

  if (handle->status & FS_FLUSHING)
    return -EBUSY;

  handle->status |= FS_IN_USE;
  lock.unlock();

  r = pwrite(fileno(handle->local_fd), buffer, size, offset);

  lock.lock();
  handle->status &= ~FS_IN_USE;
  handle->status |= FS_DIRTY;

  return r;
}

int fs::flush(const request::ptr &req, const handle_ptr &handle)
{
  string url;
  struct stat s;

  S3_DEBUG("fs::flush", "file %s needs to be written.\n", handle->path.c_str());

  url = _bucket + "/" + util::url_encode(handle->path);

  if (fflush(handle->local_fd))
    return -errno;

  if (fstat(fileno(handle->local_fd), &s))
    return -errno;

  S3_DEBUG("fs::flush", "writing %zu bytes to path %s.\n", static_cast<size_t>(s.st_size), handle->path.c_str());

  rewind(handle->local_fd);

  req->init(HTTP_PUT);
  req->set_url(url);
  req->set_header("Content-Type", handle->content_type);
  req->set_header("Content-MD5", util::compute_md5_base64(handle->local_fd));

  // TODO: set mtime?

  for (string_map::const_iterator itor = handle->metadata.begin(); itor != handle->metadata.end(); ++itor)
    req->set_header(itor->first, itor->second);

  req->set_input_file(handle->local_fd, s.st_size);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}
*/

int fs::remove_object(const request::ptr &req, const object::ptr &obj)
{
  req->init(HTTP_DELETE);
  req->set_url(obj->get_url());

  // TODO: obviously (OBVIOUSLY!) this should check if obj is a directory

  req->run();

  _object_cache.remove(obj->get_path());

  return (req->get_response_code() == 204) ? 0 : -EIO;
}

ASYNC_DEF(fs, remove_object, const std::string &path)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache.get(req, path, HINT_NONE);

  return obj ? remove_object(req, obj) : -ENOENT;
}
