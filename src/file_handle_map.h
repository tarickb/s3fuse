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

    #define HC_INVOKE(handle, cmd, ...) \
      do { \
        boost::mutex::scoped_lock lock(_mutex); \
        handle_map::const_iterator itor = _handle_map.find(handle); \
        handle_container::ptr hc; \
        int r = 0; \
        \
        if (itor == _handle_map.end()) \
          return -EINVAL; \
        \
        hc = itor->second; \
        hc->add_ref(NULL); \
        lock.unlock(); \
        \
        try { \
          r = hc->get_file()->cmd(__VA_ARGS__); \
        } catch (...) { r = -ECANCELED; } \
        \
        lock.lock(); \
        hc->release(); \
        \
        return r; \
      } while (0)

    inline int truncate(object_handle handle, off_t offset) { HC_INVOKE(handle, truncate, offset); }
    inline int flush(object_handle handle) { HC_INVOKE(handle, flush); }
    inline int read(object_handle handle, char *buffer, size_t size, off_t offset) { HC_INVOKE(handle, read, buffer, size, offset); }
    inline int write(object_handle handle, const char *buffer, size_t size, off_t offset) { HC_INVOKE(handle, write, buffer, size, offset); }

    #undef HC_INVOKE

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
