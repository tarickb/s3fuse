#include "file.h"
#include "logger.h"
#include "request.h"
#include "service.h"
#include "xattr_reference.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  object * checker(const string &path, const request::ptr &req)
  {
    S3_LOG(LOG_DEBUG, "file::checker", "testing [%s]\n", path.c_str());

    return new file(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 1000);
}

file::file(const string &path)
  : object(path),
    _status(0),
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

  req->set_header(meta_prefix + "s3fuse-md5", _md5);
  req->set_header(meta_prefix + "s3fuse-md5-etag", _md5_etag);
}

int file::open(uint64_t *handle)
{
  mutex::scoped_lock lock(_mutex);

  if (_ref_count == 0) {
    if (_status != 0) {
      S3_LOG(LOG_WARNING, "file::open", "attempt to open busy file [%s]\n", _path.c_str());
      return -EBUSY;
    }

    _status = FS_DOWNLOADING;
    // TODO: post download request
  }

  _ref_count++;
  *handle = reinterpret_cast<uint64_t>(this);

  return 0;
}

int file::release()
{
  mutex::scoped_lock lock(_mutex);

  if (_ref_count == 0) {
    S3_LOG(LOG_WARNING, "file::release", "attempt to release file [%s] with zero ref-count\n", _path.c_str());
    return -EINVAL;
  }

  _ref_count--;

  return 0;
}
