/*
 * mutexes.h
 * -------------------------------------------------------------------------
 * Common object/open-file mutexes and conditions.
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

#ifndef S3_MUTEXES_H
#define S3_MUTEXES_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class mutexes
  {
  public:
    typedef boost::shared_ptr<mutexes> ptr;

    inline boost::mutex & get_object_metadata_mutex() { return _object_metadata_mutex; }
    inline boost::mutex & get_file_status_mutex() { return _file_status_mutex; }
    inline boost::condition & get_file_status_condition() { return _file_status_condition; }

  private:
    boost::mutex _object_metadata_mutex;
    boost::mutex _file_status_mutex;
    boost::condition _file_status_condition;
  };
}

#endif
