/*
 * threads/async_handle.h
 * -------------------------------------------------------------------------
 * Asynchronous event handles (wait and callback).
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

#ifndef S3_THREADS_ASYNC_HANDLE_H
#define S3_THREADS_ASYNC_HANDLE_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  namespace threads
  {
    class async_handle
    {
    public:
      typedef boost::shared_ptr<async_handle> ptr;

      inline virtual ~async_handle()
      {
      }

      virtual void complete(int return_code) = 0;
    };

    class wait_async_handle : public async_handle
    {
    public:
      typedef boost::shared_ptr<wait_async_handle> ptr;

      inline wait_async_handle()
        : _return_code(0),
          _done(false)
      {
      }

      inline virtual ~wait_async_handle()
      {
      }

      inline virtual void complete(int return_code)
      {
        boost::mutex::scoped_lock lock(_mutex);

        _return_code = return_code;
        _done = true;

        _condition.notify_all();
      }

      inline int wait()
      {
        boost::mutex::scoped_lock lock(_mutex);

        while (!_done)
          _condition.wait(lock);

        return _return_code;
      }

    private:
      boost::mutex _mutex;
      boost::condition _condition;

      int _return_code;
      bool _done;
    };

    class callback_async_handle : public async_handle
    {
    public:
      typedef boost::shared_ptr<callback_async_handle> ptr;
      typedef boost::function1<void, int> callback_function;

      inline callback_async_handle(const callback_function &cb)
        : _cb(cb)
      {
      }

      inline virtual ~callback_async_handle()
      {
      }

      inline virtual void complete(int return_code)
      {
        _cb(return_code);
      }

    private:
      callback_function _cb;
    };
  }
}

#endif
