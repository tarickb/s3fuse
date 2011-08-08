#ifndef S3_FILE_HANDLE_MAP_H
#define S3_FILE_HANDLE_MAP_H

#include <stdint.h>

#include <map>
#include <boost/smart_ptr.hpp>

#include "handle_container.h"
#include "locked_object.h"

namespace s3
{
  // todo: rename!
  class file_handle_map
  {
  public:
    typedef boost::shared_ptr<file_handle_map> ptr;

    file_handle_map();
    ~file_handle_map();

    int open(const locked_object::ptr &file, object_handle *handle);
    int release(object_handle handle);

    int truncate(object_handle handle, off_t offset);
    int flush(object_handle handle);
    int read(object_handle handle, char *buffer, size_t size, off_t offset);
    int write(object_handle handle, const char *buffer, size_t size, off_t offset);

  private:
    typedef std::map<object_handle, handle_container::ptr> handle_map;
    typedef std::map<const object *, handle_container::ptr> object_map;

    boost::mutex _mutex;
    object_map _object_map;
    handle_map _handle_map;
    object_handle _next_handle;
  };
}

#endif
