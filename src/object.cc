#include "object.hh"
#include "request.hh"

using namespace s3;

namespace
{
  const int    BLOCK_SIZE          = 512;
  const char  *AMZ_META_PREFIX     = "x-amz-meta-";
  const size_t AMZ_META_PREFIX_LEN = sizeof(AMZ_META_PREFIX);

  int g_default_uid  = 1000;
  int g_default_gid  = 1000;
  int g_default_mode = 0755;
  int g_expiry_in_s  = 120;
}

void object::request_init()
{
  memset(&_stat, 0, sizeof(_stat));

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
    _stat.st_mode = long_value;
  else if (key == "x-amz-meta-s3fuse-uid")
    _stat.st_uid = long_value;
  else if (key == "x-amz-meta-s3fuse-gid")
    _stat.st_gid = long_value;
  else if (key == "x-amz-meta-s3fuse-mtime")
    _stat.st_mtime = long_value;
  else if (strncmp(key.c_str(), AMZ_META_PREFIX, AMZ_META_PREFIX_LEN) == 0)
    _metadata[key] = value;
}

void object::request_process_response(long code, time_t last_modified)
{
  if (code != 200)
    return;

  _stat.st_mode   = (_stat.st_mode == 0) ? g_default_mode : _stat.st_mode;
  _stat.st_uid    = (_stat.st_uid  == 0) ? g_default_uid  : _stat.st_uid;
  _stat.st_gid    = (_stat.st_gid  == 0) ? g_default_gid  : _stat.st_gid;

  _stat.st_mode  |= ((hints & OH_IS_DIRECTORY) ? S_IFDIR : S_IFREG);
  _stat.st_nlink  = 1; // laziness (see FUSE FAQ re. find)

  // the object could have been modified by someone else
  if (last_modified > _stat.st_mtime)
    _stat.st_mtime = last_modified;

  if ((hints & OH_IS_DIRECTORY) == 0)
    _stat.st_blocks = (_stat->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // setting _expiry > 0 makes this object valid
  _expiry = time(NULL) + g_expiry_in_s;
}
