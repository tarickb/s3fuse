#include <list>
#include <boost/lexical_cast.hpp>

#include "logging.hh"
#include "object.hh"
#include "open_file_cache.hh"
#include "request.hh"
#include "thread_pool.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const size_t CHUNK_SIZE = 128 * 1024; // 128 KiB

  int g_max_retries = 3;

  struct transfer_part
  {
    off_t offset;
    size_t size;
    int retry_count;
    async_handle handle;

    inline transfer_part(off_t offset_, size_t size_) : offset(offset_), size(size_), retry_count(0) { }
  };

  enum transfer_result
  {
    TR_SUCCESS,
    TR_TEMPORARY_FAILURE,
    TR_PERMANENT_FAILURE
  };
}

open_file_cache::open_file_cache(const thread_pool::ptr &pool)
  : _pool(pool),
    _next_handle(0)
{
}

int open_file_cache::download_file_single(const request::ptr &req, const object::ptr &obj, FILE *f)
{
  long rc;

  req->init(HTTP_GET);
  req->set_url(obj->get_url());
  req->set_output_file(f);

  req->run();
  rc = req->get_response_code();

  if (rc == 200)
    return 0;
  else if (rc == 404)
    return -ENOENT;
  else
    return -EIO;
}

int open_file_cache::__download_part(const request::ptr &req, const string &url, FILE *f, off_t offset, size_t size, string *etag)
{
  long rc;

  req->init(HTTP_GET);
  req->set_url(url);
  req->set_output_file(f, offset);
  req->set_header("Range", 
    string("bytes=") + 
    lexical_cast<string>(offset) + 
    string("-") + 
    lexical_cast<string>(offset + size));

  req->run();
  rc = req->get_response_code();

  if (rc == 500 || rc == 503)
    return TR_TEMPORARY_FAILURE;
  else if (rc != 206)
    return TR_PERMANENT_FAILURE;

  if (etag)
    *etag = req->get_response_header("ETag");

  return TR_SUCCESS;
}

int open_file_cache::download_file_multi(const request::ptr &req, const object::ptr &obj, FILE *f)
{
  off_t offset = 0;
  size_t size = obj->get_size();
  string url = obj->get_url();
  string expected_md5, computed_md5;
  list<transfer_part> parts;

  while (size) {
    size_t part_size = (size > CHUNK_SIZE) ? CHUNK_SIZE : size;
    transfer_part part(offset, part_size);

    S3_DEBUG("open_file_cache::download_file_multi", "downloading %zu bytes at %zd for file %s.\n", part.size, part.offset, url.c_str());

    part.handle = _pool->post(bind(&open_file_cache::__download_part, this, _1, url, f, part.offset, part.size, (part.offset == 0) ? &expected_md5 : NULL));
    parts.push_back(part);

    size -= part_size;
    offset += part_size;
  }

  while (!parts.empty()) {
    transfer_part part = parts.front();
    transfer_result r;

    parts.pop_front();
    r = static_cast<transfer_result>(_pool->wait(part.handle));

    S3_DEBUG("open_file_cache::download_file_multi", "status %i for %zu bytes at %zd for file %s.\n", r, part.size, part.offset, url.c_str());

    if (r == TR_PERMANENT_FAILURE) {
      return -EIO;

    } else if (r == TR_TEMPORARY_FAILURE) {
      part.retry_count++;

      if (part.retry_count > g_max_retries)
        return -EIO;

      part.handle = _pool->post(bind(&open_file_cache::__download_part, this, _1, url, f, part.offset, part.size, (part.offset == 0) ? &expected_md5 : NULL));
      parts.push_back(part);
    }
  }

  computed_md5 = "\"" + util::compute_md5(fileno(f), 0, 0, MOT_HEX) + "\"";

  if (computed_md5 != expected_md5) {
    S3_DEBUG(
      "open_file_cache::download_file_multi", 
      "md5 mismatch. expected %s, got %s.\n", 
      expected_md5.c_str(),
      computed_md5.c_str());

    return -EIO;
  }

  return 0;
}

int open_file_cache::open(const request::ptr &req, const object::ptr &obj, uint64_t *handle)
{
  FILE *temp_file;
  int r;

  // TODO: check if file is already open, and return existing handle
  // TODO: use FDs rather than FILE *

  if (!obj)
    return -ENOENT;

  temp_file = tmpfile();

  if (!temp_file)
    return -errno;

  // TODO: run download in parallel?

  if (obj->get_size() > CHUNK_SIZE)
    r = download_file_multi(req, obj, temp_file);
  else
    r = download_file_single(req, obj, temp_file);

  if (r) {
    fclose(temp_file);
    return r;
  }

  fflush(temp_file);

  {
    mutex::scoped_lock lock(_mutex);

    if (obj->get_local_file()) {
      fclose(temp_file);
      return -EBUSY;
    }

    obj->set_local_file(temp_file);

    *handle = _next_handle++;
    _files[*handle].reset(new open_file(obj));
  }

  S3_DEBUG("open_file_cache::open", "opened file %s with handle %" PRIu64 ".\n", obj->get_path().c_str(), *handle);

  return 0;
}

int open_file_cache::flush(const request_ptr &req, uint64_t handle, bool close_when_done)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r = 0;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_IN_USE)
    return -EBUSY;

  if (file->status & FS_FLUSHING)
    return close_when_done ? -EBUSY : 0; // another thread is flushing this file, so don't report an error

  file->status |= FS_FLUSHING;
  lock.unlock();

  if (file->status & FS_DIRTY)
    r = upload_file_single(req, file->obj);

  lock.lock();
  file->status &= ~FS_FLUSHING;

  if (!r) {
    file->status &= ~FS_DIRTY;

    if (close_when_done) {
      _files.erase(handle);

      file->obj->set_local_file(NULL);
      file->obj->invalidate();
    }
  }

  return r;
}

int open_file_cache::write(uint64_t handle, const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_FLUSHING)
    return -EBUSY;

  file->status |= FS_IN_USE;
  lock.unlock();

  r = pwrite(fileno(file->obj->get_local_file()), buffer, size, offset);

  lock.lock();
  file->status &= ~FS_IN_USE;
  file->status |= FS_DIRTY;

  return r;
}

int open_file_cache::read(uint64_t handle, char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_FLUSHING)
    return -EBUSY;

  file->status |= FS_IN_USE;
  lock.unlock();

  r = pread(fileno(file->obj->get_local_file()), buffer, size, offset);

  lock.lock();
  file->status &= ~FS_IN_USE;

  return r;
}

int open_file_cache::upload_file_single(const request::ptr &req, const object::ptr &obj)
{
  struct stat s;
  FILE *f = obj->get_local_file();

  S3_DEBUG("open_file_cache::upload_file_single", "file %s needs to be written.\n", obj->get_path().c_str());

  if (fflush(f))
    return -errno;

  if (fstat(fileno(f), &s))
    return -errno;

  S3_DEBUG("open_file_cache::upload_file_single", "writing %zu bytes to path %s.\n", static_cast<size_t>(s.st_size), obj->get_path().c_str());

  rewind(f);

  // TODO: use multipart uploads?

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);
  req->set_header("Content-MD5", util::compute_md5(fileno(f)));
  req->set_input_file(f, s.st_size);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}
