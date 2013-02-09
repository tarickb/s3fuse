/*
 * threads/work_item.h
 * -------------------------------------------------------------------------
 * Pool work item.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
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

#ifndef S3_THREADS_WORK_ITEM_H
#define S3_THREADS_WORK_ITEM_H

#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace threads
  {
    class async_handle;

    class work_item
    {
    public:
      typedef boost::function1<int, boost::shared_ptr<base::request> > worker_function;

      inline work_item()
        : _retries(-1)
      {
      }

      inline work_item(const worker_function &function, const boost::shared_ptr<async_handle> &ah, int retries)
        : _function(function), 
          _ah(ah),
          _retries(retries)
      {
      }

      inline bool is_valid() const { return _ah; }
      inline bool has_retries_left() const { return _retries > 0; }

      inline const boost::shared_ptr<async_handle> & get_ah() const { return _ah; }
      inline const worker_function & get_function() const { return _function; }

      inline work_item decrement_retry_counter() const
      {
        return work_item(_function, _ah, _retries - 1);
      }

    private:
      worker_function _function;
      boost::shared_ptr<async_handle> _ah;
      int _retries;
    };
  }
}

#endif
