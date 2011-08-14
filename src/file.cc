#include "file.h"
#include "logger.h"
#include "request.h"

using namespace boost;
using namespace std;

using namespace s3;

file::file(const string &path)
  : object(path),
    _fd(-1)
{
  init();
}

object_type file::get_type()
{
  return OT_FILE;
}

mode_t file::get_mode()
{
  return S_IFREG;
}

int file::set_metadata(const string &key, const string &value, int flags)
{
  if (key == "__md5__")
    return -EINVAL;

  return object::set_metadata(key, value, flags);
}

void file::get_metadata_keys(vector<string> *keys)
{
  object::get_metadata_keys(keys);

  keys->push_back("__md5__");
}

int file::get_metadata(const string &key, string *value)
{
  if (key == "__md5__") {
    mutex::scoped_lock lock(get_mutex());

    *value = _md5;
    return 0;
  }

  return object::get_metadata(key, value);
}

void file::build_process_header(const request::ptr &req, const string &key, const string &value)
{
  const string &meta_prefix = service::get_meta_prefix();

  if (key == (meta_prefix + "s3fuse-md5"))
    _md5 = value;
  else if (key == (meta_prefix + "s3fuse-md5-etag"))
    _md5_etag = value;
  else
    object::build_process_header(req, key, value);
}

void file::build_finalize(const request::ptr &req)
{
  const string &etag = get_etag();

  // this workaround is for multipart uploads, which don't get a valid md5 etag
  if (!util::is_valid_md5(_md5))
    _md5.clear();

  if ((_md5_etag != etag || _md5.empty()) && util::is_valid_md5(etag))
    _md5 = etag;

  _md5_etag = etag;

  object::build_finalize(req);
}

void file::set_meta_headers(const request::ptr &req)
{
  const string &meta_prefix = service::get_meta_prefix();

  object::set_meta_headers(req);

  {
    mutex::scoped_lock lock(get_mutex());

    req->set_header(meta_prefix + "s3fuse-md5", _md5);
    req->set_header(meta_prefix + "s3fuse-md5-etag", _md5_etag);
  }
}

void file::copy_stat(struct stat *s)
{
  int fd = _fd; // local copy

  object::copy_stat(s);

  if (fd != -1) {
    struct stat temp;

    if (fstat(fd, &temp) == 0)
      s->st_size = temp.st_size;
  }
}

int file::open()
{
  char temp_name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(temp_name);
  unlink(temp_name);

  S3_LOG(LOG_DEBUG, "file::open", "opening [%s] in [%s].\n", get_path().c_str(), temp_name);

  if (_fd == -1)
    throw runtime_error("error calling mkstemp()");

  if (ftruncate(_fd, get_size()) != 0) {
    ::close(_fd);
    _fd = -1;

    throw runtime_error("failed to truncate temporary file.");
  }
}

int file::close()
{
  ::close(_fd);
  _fd = -1;
}

int file::flush()
{
}
