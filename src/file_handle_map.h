#ifndef S3_FILE_HANDLE_MAP_H
#define S3_FILE_HANDLE_MAP_H

#include <stdint.h>

#include <map>
#include <boost/smart_ptr.hpp>

#include "file.h"
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

    #define LO_INVOKE(handle, cmd, ...) \
      do { \
        boost::mutex::scoped_lock lock(_mutex); \
        handle_map::const_iterator itor = _handle_map.find(handle); \
        locked_object::ptr lo; \
        int r = 0; \
        \
        if (itor == _handle_map.end()) \
          return -EINVAL; \
        \
        lo = itor->second; \
        get_object(lo)->add_ref(NULL); \
        lock.unlock(); \
        \
        try { \
          r = get_file(lo)->cmd(__VA_ARGS__); \
        } catch (...) { r = -ECANCELED; } \
        \
        lock.lock(); \
        get_object(lo)->release(); \
        \
        return r; \
      } while (0)

    inline int truncate(object_handle handle, off_t offset) { LO_INVOKE(handle, truncate, offset); }
    inline int flush(object_handle handle) { LO_INVOKE(handle, flush); }
    inline int read(object_handle handle, char *buffer, size_t size, off_t offset) { LO_INVOKE(handle, read, buffer, size, offset); }
    inline int write(object_handle handle, const char *buffer, size_t size, off_t offset) { LO_INVOKE(handle, write, buffer, size, offset); }

    #undef LO_INVOKE

  private:
    typedef std::map<object_handle, locked_object::ptr> handle_map;
    typedef std::map<const object *, locked_object::ptr> object_map;

    inline object * get_object(const locked_object::ptr &lo) { return lo->get().get(); }
    inline file * get_file(const locked_object::ptr &lo) { return dynamic_cast<file *>(get_object(lo)); }

    boost::mutex _mutex;
    object_map _object_map;
    handle_map _handle_map;
    object_handle _next_handle;
  };
}

#endif
