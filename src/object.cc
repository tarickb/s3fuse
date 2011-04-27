#include <string.h>

#include "object.hh"
#include "request.hh"

using namespace std;

using namespace s3;

namespace
{
  const int    BLOCK_SIZE           = 512;
  const char  *AMZ_META_PREFIX      = "x-amz-meta-";
  const size_t AMZ_META_PREFIX_LEN  = sizeof(AMZ_META_PREFIX);
  const char  *SYMLINK_CONTENT_TYPE = "text/symlink";

  int    g_default_uid          = 1000;
  int    g_default_gid          = 1000;
  int    g_default_mode         = 0755;
  int    g_expiry_in_s          = 3 * 60; // 3 minutes
  string g_default_content_type = "binary/octet-stream";

  mode_t get_mode_by_type(object_type type)
  {
    if (type == OT_FILE)
      return S_IFREG;

    if (type == OT_DIRECTORY)
      return S_IFDIR;

    if (type == OT_SYMLINK)
      return S_IFLNK;

    return 0;
  }
}

// TODO: remove
/*

what determines an object's type?
options:
  1. metadata header (x-amz-meta-s3fuse-type)
  2. content type
     -- not compatible with other s3 tools
  3. heuristics
     a. if path ends in "/", it's a directory
     b. if content type is "text/symlink", it's a symlink
     c. otherwise, it's a file

*/

void object::set_defaults(object_type type)
{
  memset(&_stat, 0, sizeof(_stat));

  _stat.st_uid = g_default_uid;
  _stat.st_gid = g_default_gid;
  _stat.st_mode = g_default_mode | get_mode_by_type(type);
  _stat.st_nlink  = 1; // laziness (see FUSE FAQ re. find)
  _stat.st_mtime = time(NULL);

  _type = type;
  _content_type = ((type == OT_SYMLINK) ? string(SYMLINK_CONTENT_TYPE) : g_default_content_type);
  _etag.clear();
  _expiry = time(NULL) + g_expiry_in_s;
  _local_file = NULL;
  _metadata.clear();
}

void object::set_mode(mode_t mode)
{
  mode = mode & ~S_IFMT;

  if (mode == 0)
    mode = g_default_mode;

  _stat.st_mode = (_stat.st_mode & S_IFMT) | mode;
}

void object::request_init()
{
  memset(&_stat, 0, sizeof(_stat));

  _type = (_hints & OH_IS_DIRECTORY) ? OT_DIRECTORY : OT_FILE;
  _content_type.clear();
  _etag.clear();
  _expiry = 0;
  _local_file = NULL;
  _metadata.clear();
}

void object::request_process_header(const std::string &key, const std::string &value)
{
  long long_value = strtol(value.c_str(), NULL, 0);

  if (key == "Content-Type")
    _content_type = value;
  else if (key == "ETag")
    _etag = value;
  else if (key == "Content-Length")
    _stat.st_size = long_value;
  else if (key == "x-amz-meta-s3fuse-mode")
    _stat.st_mode = long_value & ~S_IFMT;
  else if (key == "x-amz-meta-s3fuse-uid")
    _stat.st_uid = long_value;
  else if (key == "x-amz-meta-s3fuse-gid")
    _stat.st_gid = long_value;
  else if (key == "x-amz-meta-s3fuse-mtime")
    _stat.st_mtime = long_value;
  else if (key == "x-amz-meta-s3fuse-type")
    _type = object_type(long_value);
  else if (strncmp(key.c_str(), AMZ_META_PREFIX, AMZ_META_PREFIX_LEN) == 0)
    _metadata[key.substr(AMZ_META_PREFIX_LEN)] = value;
}

void object::request_process_response(request *req)
{
  if (req->get_response_code() != 200)
    return;

  _stat.st_mode   = (_stat.st_mode == 0) ? g_default_mode : _stat.st_mode;
  _stat.st_uid    = (_stat.st_uid  == 0) ? g_default_uid  : _stat.st_uid;
  _stat.st_gid    = (_stat.st_gid  == 0) ? g_default_gid  : _stat.st_gid;

  _stat.st_mode  |= get_mode_by_type(_type);
  _stat.st_nlink  = 1; // laziness (see FUSE FAQ re. find)

  // the object could have been modified by someone else
  if (req->get_last_modified() > _stat.st_mtime)
    _stat.st_mtime = req->get_last_modified();

  if (_type == OT_FILE)
    _stat.st_blocks = (_stat.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // setting _expiry > 0 makes this object valid
  _expiry = time(NULL) + g_expiry_in_s;
}

void object::request_set_meta_headers(request *req)
{
  char buf[16];
  const struct stat *s = get_stats();
  string prefix(AMZ_META_PREFIX);

  snprintf(buf, 16, "%#o", s->st_mode);
  req->set_header("x-amz-meta-s3fuse-mode", buf);

  snprintf(buf, 16, "%i", s->st_uid);
  req->set_header("x-amz-meta-s3fuse-uid", buf);

  snprintf(buf, 16, "%i", s->st_gid);
  req->set_header("x-amz-meta-s3fuse-gid", buf);

  snprintf(buf, 16, "%li", s->st_mtime);
  req->set_header("x-amz-meta-s3fuse-mtime", buf);

  req->set_header("Content-Type", _content_type);

  for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    req->set_header(prefix + itor->first, itor->second);
}
