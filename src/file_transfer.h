/*
 * transfer_manager.h
 * -------------------------------------------------------------------------
 * Single- and multi-part uploads/downloads.
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

#ifndef S3_TRANSFER_MANAGER_H
#define S3_TRANSFER_MANAGER_H

#include <boost/shared_ptr.hpp>

#include "thread_pool.h"

namespace s3
{
  class file;
  class request;

  class transfer_manager
  {
  public:
    typedef boost::function3<ssize_t, const char *, size_t, off_t> writer;
    typedef boost::function3<ssize_t, char *, size_t, off_t> reader;

    inline static int download(const boost::shared_ptr<file> &file)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&transfer_manager::download, this, _1, file));
    }

    inline static void download(const boost::shared_ptr<file> &file, const callback_async_handle::callback_function &cb)
    {
      thread_pool::post(thread_pool::PR_FG, boost::bind(&transfer_manager::download, this, _1, file), cb);
    }

    inline static int upload(const boost::shared_ptr<file> &file)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&transfer_manager::upload, this, _1, file));
    }

    inline static void upload(const boost::shared_ptr<file> &file, const callback_async_handle::callback_function &cb)
    {
      thread_pool::post(thread_pool::PR_FG, boost::bind(&transfer_manager::upload, this, _1, file), cb);
    }

  private:
    static int download(const boost::shared_ptr<request> &req, const boost::shared_ptr<file> &f);
    static int upload(const boost::shared_ptr<request> &req, const boost::shared_ptr<file> &f);
  };
}

#endif
