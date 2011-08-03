#ifndef S3_FILE_HANDLE_MAP_H
#define S3_FILE_HANDLE_MAP_H

#include <stdint.h>

#include <map>
#include <boost/smart_ptr.hpp>

#include "locked_object.h"

namespace s3
{
  class file_handle_map
  {
  public:
    typedef boost::shared_ptr<file_handle_map> ptr;

    file_handle_map();
    ~file_handle_map();

    int open(const locked_object::ptr &file, uint64_t *handle);
    int release(uint64_t handle);

    boost::shared_ptr<open_file> get(uint64_t handle);

  private:
    typedef std::map<uint64_t, locked_object::ptr> object_map;

    boost::mutex _mutex;
    object_map _map;
    uint64_t _next_handle;
  };
}

#endif
