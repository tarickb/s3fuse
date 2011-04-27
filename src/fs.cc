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
  const int BLOCK_SIZE = 512;

  int g_default_uid  = 1000;
  int g_default_gid  = 1000;
  int g_default_mode = 0755;

  void get_object_metadata(const request::ptr &req, mode_t *mode, uid_t *uid, gid_t *gid)
  {
    const string &mode_str = req->get_response_header("x-amz-meta-s3fuse-mode");
    const string & uid_str = req->get_response_header("x-amz-meta-s3fuse-uid" );
    const string & gid_str = req->get_response_header("x-amz-meta-s3fuse-gid" );

    *mode = (mode_str.empty() ? g_default_mode : strtol(mode_str.c_str(), NULL, 0));
    * uid = ( uid_str.empty() ? g_default_uid  : strtol( uid_str.c_str(), NULL, 0));
    * gid = ( gid_str.empty() ? g_default_gid  : strtol( gid_str.c_str(), NULL, 0));
  }

  void set_object_metadata(const request::ptr &req, mode_t mode, uid_t uid, gid_t gid)
  {
    char buf[16];

    snprintf(buf, 16, "%#o", mode);
    req->set_header("x-amz-meta-s3fuse-mode", buf);

    snprintf(buf, 16, "%i", uid);
    req->set_header("x-amz-meta-s3fuse-uid", buf);

    snprintf(buf, 16, "%i", gid);
    req->set_header("x-amz-meta-s3fuse-gid", buf);
  }
}

fs::fs(const string &bucket)
  : _bucket(string("/") + util::url_encode(bucket)),
    _prefix_query("/ListBucketResult/CommonPrefixes/Prefix"),
    _key_query("/ListBucketResult/Contents"),
    _fg_thread_pool("fs-fg"),
    _bg_thread_pool("fs-bg"),
    _next_open_file_handle(0)
{
}

fs::~fs()
{
}

ASYNC_DEF(prefill_stats, const string &path, int hints)
{
  struct stat s;

  return __get_stats(req, path, NULL, &s, hints);
}

ASYNC_DEF(get_stats, const string &path, string *etag, struct stat *s, int hints)
{
  bool is_directory = (hints == HINT_NONE || hints & HINT_IS_DIR);

  ASSERT_NO_TRAILING_SLASH(path);

  if (_stats_cache.get(path, etag, s))
    return 0;

  req->init(HTTP_HEAD);

  if (hints == HINT_NONE || hints & HINT_IS_DIR) {
    // see if the path is a directory (trailing /) first
    req->set_url(_bucket + "/" + util::url_encode(path) + "/", "");
    req->run();
  }

  if (hints & HINT_IS_FILE || req->get_response_code() != 200) {
    // it's not a directory
    is_directory = false;
    req->set_url(_bucket + "/" + util::url_encode(path), "");
    req->run();
  }

  if (req->get_response_code() != 200)
    return -ENOENT;

  // TODO: check for delete markers?

  if (s) {
    const string &length = req->get_response_header("Content-Length");

    memset(s, 0, sizeof(*s));
    get_object_metadata(req, &s->st_mode, &s->st_uid, &s->st_gid);

    // TODO: support symlinks?
    s->st_mode |= (is_directory ? S_IFDIR : S_IFREG);
    s->st_size = strtol(length.c_str(), NULL, 0);
    s->st_nlink = 1; // laziness (see FUSE FAQ re. find)
    s->st_mtime =  req->get_last_modified();

    if (!is_directory)
      s->st_blocks = (s->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    _stats_cache.update(path, req->get_response_header("ETag"), s);
  }

  if (etag)
    *etag = req->get_response_header("ETag");

  return 0;
}

ASYNC_DEF(rename_object, const std::string &from, const std::string &to)
{
  struct stat s;
  string etag, from_url, to_url;
  int r;

  ASSERT_NO_TRAILING_SLASH(from);
  ASSERT_NO_TRAILING_SLASH(to);

  r = __get_stats(req, from, &etag, &s, HINT_NONE);

  if (r)
    return r;

  // TODO: support directories
  if (S_ISDIR(s.st_mode))
    return -EINVAL;

  r = __get_stats(req, to, NULL, NULL, HINT_NONE);

  if (r != -ENOENT)
    return (r == 0) ? -EEXIST : r;

  from_url = _bucket + "/" + util::url_encode(from);
  to_url = _bucket + "/" + util::url_encode(to);

  req->init(HTTP_PUT);
  req->set_url(to_url, "");
  req->set_header("Content-Type", "binary/octet-stream");
  req->set_header("x-amz-copy-source", from_url);
  req->set_header("x-amz-copy-source-if-match", etag);
  req->set_header("x-amz-metadata-directive", "COPY");

  req->run();

  if (req->get_response_code() != 200)
    return -EIO;

  return __remove_object(req, from, HINT_IS_FILE);
}

ASYNC_DEF(change_metadata, const std::string &path, mode_t mode, uid_t uid, gid_t gid)
{
  struct stat s;
  string etag, url;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  r = __get_stats(req, path, &etag, &s, HINT_NONE);

  if (r)
    return r;

  if (mode != mode_t(-1)) {
    S3_DEBUG("fs::change_metadata", "changing mode from %#o to %#o.\n", s.st_mode, mode);
    s.st_mode = mode;
  }

  if (uid != uid_t(-1)) {
    S3_DEBUG("fs::change_metadata", "changing user from %i to %i.\n", s.st_uid, uid);
    s.st_uid = uid;
  }

  if (gid != gid_t(-1)) {
    S3_DEBUG("fs::change_metadata", "changing group from %i to %i.\n", s.st_gid, gid);
    s.st_gid = gid;
  }

  // TODO: reuse with create_object
  url = _bucket + "/" + util::url_encode(path);

  if (S_ISDIR(s.st_mode))
    url += "/";

  req->init(HTTP_PUT);
  req->set_url(url, "");
  req->set_header("Content-Type", "binary/octet-stream");
  req->set_header("x-amz-copy-source", url);
  req->set_header("x-amz-copy-source-if-match", etag);
  req->set_header("x-amz-metadata-directive", "REPLACE");

  set_object_metadata(req, s.st_mode, s.st_uid, s.st_gid);

  req->run();

  if (req->get_response_code() != 200) {
    S3_DEBUG("fs::change_metadata", "response: %s\n", req->get_response_data().c_str());
    return -EIO;
  }

  _stats_cache.remove(path);

  return 0;
}

ASYNC_DEF(read_directory, const std::string &_path, fuse_fill_dir_t filler, void *buf)
{
  size_t path_len;
  string marker = "";
  bool truncated = true;
  string path;

  // TODO: use worker threads to call get_stats in parallel!
  // TODO: openssl locking?

  ASSERT_NO_TRAILING_SLASH(_path);

  if (!_path.empty())
    path = _path + "/";

  path_len = path.size();

  req->init(HTTP_GET);

  while (truncated) {
    xml_document doc;
    xml_parse_result res;
    xpath_node_set prefixes, keys;

    req->set_url(_bucket, string("delimiter=/&prefix=") + util::url_encode(path) + "&marker=" + marker);
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

      ASYNC_CALL_NONBLOCK_BG(prefill_stats, full_path, HINT_IS_DIR);
      filler(buf, relative_path.c_str(), NULL, 0);
    }

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *full_path_cs = itor->node().child_value("Key");

      if (strcmp(path.data(), full_path_cs) != 0) {
        string relative_path(full_path_cs + path_len);
        string full_path(full_path_cs);

        S3_DEBUG("fs::read_directory", "found key [%s]\n", relative_path.c_str());

        ASYNC_CALL_NONBLOCK_BG(prefill_stats, full_path, HINT_IS_FILE);
        filler(buf, relative_path.c_str(), NULL, 0);
      }
    }
  }

  return 0;
}

ASYNC_DEF(create_object, const std::string &path, mode_t mode)
{
  string url;

  ASSERT_NO_TRAILING_SLASH(path);

  url = _bucket + "/" + util::url_encode(path);

  if (__get_stats(req, path, NULL, NULL, HINT_NONE) == 0) {
    S3_DEBUG("fs::create_object", "attempt to overwrite object at path %s.\n", path.c_str());
    return -EEXIST;
  }

  if (S_ISDIR(mode))
    url += "/";

  req->init(HTTP_PUT);
  req->set_url(url, "");
  req->set_header("Content-Type", "binary/octet-stream");

  if ((mode & ~S_IFMT) == 0) {
    S3_DEBUG("fs::create_object", "no mode specified, using default.\n");
    mode |= g_default_mode;
  }

  // TODO: use parent?
  set_object_metadata(req, mode, g_default_uid, g_default_gid);

  req->run();

  return 0;
}

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
  req->set_url(url, "");
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
  req->set_url(url, "");
  req->set_header("Content-Type", handle->content_type);
  req->set_header("Content-MD5", util::compute_md5_base64(handle->local_fd));

  // TODO: set mtime?

  for (string_map::const_iterator itor = handle->metadata.begin(); itor != handle->metadata.end(); ++itor)
    req->set_header(itor->first, itor->second);

  req->set_input_file(handle->local_fd, s.st_size);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}

ASYNC_DEF(remove_object, const std::string &path, int hints)
{
  string url;

  ASSERT_NO_TRAILING_SLASH(path);

  url = _bucket + "/" + util::url_encode(path);

  // TODO: check if children exist first!
  if (hints & HINT_IS_DIR)
    url += "/";

  req->init(HTTP_DELETE);
  req->set_url(url, "");

  req->run();

  _stats_cache.remove(path);

  return (req->get_response_code() == 204) ? 0 : -EIO;
}
