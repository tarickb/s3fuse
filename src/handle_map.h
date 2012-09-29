#ifndef S3_HANDLE_MAP_H
#define S3_HANDLE_MAP_H

#include <map>
#include <boost/thread.hpp>

#include "file.h"

namespace s3
{
  class handle_map
  {
  public:
    handle_map();

    file::lock get(uint64_t handle);

    uint64_t add_file(file *f);
    void remove_file(uint64_t handle);

  private:
    typedef std::map<uint64_t, file::lock> file_pointer_map;

    boost::mutex _mutex;
    file_pointer_map _map;

    uint64_t _next_handle;
  };
}

#endif
