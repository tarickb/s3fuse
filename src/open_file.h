/*
 * open_file.h
 * -------------------------------------------------------------------------
 * Wraps an open file.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef S3_OPEN_FILE_H
#define S3_OPEN_FILE_H

#include <stdint.h>
#include <sys/stat.h>

#include <boost/shared_ptr.hpp>

namespace s3
{
  class file_transfer;
  class mutexes;
  class object;
  class object_cache;

  class open_file
  {
  public:
    typedef boost::shared_ptr<open_file> ptr;

    ~open_file();

    inline int get_fd() { return _fd; }
    inline uint64_t get_handle() { return _handle; }

    int init();
    int cleanup();

    int truncate(off_t offset);
    int flush();
    int read(char *buffer, size_t size, off_t offset);
    int write(const char *buffer, size_t size, off_t offset);

  private:
    friend class object_cache; // for ctor, add_reference, release

    open_file(
      const boost::shared_ptr<mutexes> &mutexes, 
      const boost::shared_ptr<file_transfer> &file_transfer, 
      const boost::shared_ptr<object> &obj, 
      uint64_t handle);

    int add_reference(uint64_t *handle);
    bool release();

    boost::shared_ptr<mutexes> _mutexes;
    boost::shared_ptr<file_transfer> _file_transfer;
    boost::shared_ptr<object> _obj;
    uint64_t _handle, _ref_count;
    int _fd, _status, _error;
  };
}

#endif
