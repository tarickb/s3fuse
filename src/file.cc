#include "file.h"
#include "logger.h"
#include "request.h"
#include "service.h"
#include "thread_pool.h"
#include "xattr_reference.h"

using namespace boost;
using namespace std;

using namespace s3;

#define TEMP_NAME_TEMPLATE "/tmp/s3fuse.local-XXXXXX"

namespace
{
  object * checker(const string &path, const request::ptr &req)
  {
    S3_LOG(LOG_DEBUG, "file::checker", "testing [%s]\n", path.c_str());

    return new file(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 1000);

  int upload();
  void async_download();
}

file::file(const string &path)
  : object(path),
    _fd(-1),
    _status(0),
    _async_error(0),
    _ref_count(0)
{
}

bool file::is_valid()
{
  mutex::scoped_lock lock(_mutex);

  return _ref_count > 0 || object::is_valid();
}

void file::init(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::init(req);

  _md5 = req->get_response_header(meta_prefix + "s3fuse-md5");
  _md5_etag = req->get_response_header(meta_prefix + "s3fuse-md5-etag");

  _metadata.insert(xattr_reference::from_string("__md5__", &_md5));

  // this workaround is for multipart uploads, which don't get a valid md5 etag
  if (!util::is_valid_md5(_md5))
    _md5.clear();

  if ((_md5_etag != _etag || _md5.empty()) && util::is_valid_md5(_etag))
    _md5 = _etag;

  _md5_etag = _etag;
}

void file::set_request_headers(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::set_request_headers(req);

  req->set_header(meta_prefix + "s3fuse-md5", _md5);
  req->set_header(meta_prefix + "s3fuse-md5-etag", _md5_etag);
}

int file::download(const request::ptr &req)
{
  return 0;
}

int file::upload(const request::ptr &req)
{
  return 0;
}

void file::on_download_complete(int ret)
{
  mutex::scoped_lock lock(_mutex);

  if (_status != FS_DOWNLOADING) {
    S3_LOG(LOG_ERR, "file::download_complete", "inconsistent state for [%s]. don't know what to do.\n", get_path().c_str());
    return;
  }
}

int file::open(uint64_t *handle)
{
  mutex::scoped_lock lock(_mutex);

  if (_ref_count == 0) {
    char temp_name[] = TEMP_NAME_TEMPLATE;

    _fd = mkstemp(temp_name);
    unlink(temp_name);

    S3_LOG(LOG_DEBUG, "file::open", "opening [%s] in [%s].\n", get_path().c_str(), temp_name);

    if (_fd == -1)
      return -errno;

    if (ftruncate(_fd, _stat.st_size) != 0)
      return -errno;

    _status = FS_DOWNLOADING;

    thread_pool::get_fg()->post(
      bind(&file::download, shared_from_this(), _1),
      bind(&file::on_download_complete, shared_from_this(), _1));
  }

  *handle = reinterpret_cast<uint64_t>(this);
  _ref_count++;

  return 0;
}

int file::release()
{
  mutex::scoped_lock lock(_mutex);

  if (_ref_count == 0) {
    S3_LOG(LOG_WARNING, "file::release", "attempt to release file [%s] with zero ref-count\n", get_path().c_str());
    return -EINVAL;
  }

  _ref_count--;

  if (_ref_count == 0) {
    if (_status != 0) {
      S3_LOG(LOG_ERR, "file::release", "released file [%s] with non-quiescent status [%i].\n", get_path().c_str(), _status);
      return -EBUSY;
    }

    close(_fd);
  }

  return 0;
}

int file::flush()
{
  mutex::scoped_lock lock(_mutex);

  while (_status & (FS_DOWNLOADING | FS_UPLOADING | FS_WRITING))
    _condition.wait(lock);

  if (!(_status & FS_DIRTY)) {
    S3_LOG(LOG_DEBUG, "file::flush", "skipping flush for non-dirty file [%s].\n", get_path().c_str());
    return 0;
  }

  _status |= FS_UPLOADING;

  lock.unlock();
  _async_error = thread_pool::get_fg()->call(bind(&file::upload, shared_from_this(), _1));
  lock.lock();

  _status = 0;
  _condition.notify_all();

  return _async_error;
}

int file::write(const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);
  int r;

  while (_status & (FS_DOWNLOADING | FS_UPLOADING))
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  _status |= FS_DIRTY | FS_WRITING;

  lock.unlock();
  r = pwrite(_fd, buffer, size, offset);
  lock.lock();

  _status &= ~FS_WRITING;
  _condition.notify_all();

  return r;
}

int file::read(char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);

  while (_status & FS_DOWNLOADING)
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  lock.unlock();

  return pread(_fd, buffer, size, offset);
}
