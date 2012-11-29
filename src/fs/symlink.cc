/*
 * fs/symlink.cc
 * -------------------------------------------------------------------------
 * Symbolic link class implementation.
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

#include "base/logger.h"
#include "base/request.h"
#include "fs/symlink.h"

using boost::defer_lock;
using boost::mutex;
using std::string;

using s3::base::request;
using s3::fs::object;
using s3::fs::symlink;

namespace
{
  const string CONTENT_TYPE = "text/symlink";

  const string CONTENT_PREFIX = "SYMLINK:";
  const char * CONTENT_PREFIX_CSTR = CONTENT_PREFIX.c_str();
  const size_t CONTENT_PREFIX_LEN = CONTENT_PREFIX.size();

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new s3::fs::symlink(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 100);
}

symlink::symlink(const string &path)
  : object(path)
{
  set_content_type(CONTENT_TYPE);
  set_object_type(S_IFLNK);
}

symlink::~symlink()
{
}

void symlink::set_request_body(const request::ptr &req)
{
  mutex::scoped_lock lock(_mutex);

  req->set_input_buffer(CONTENT_PREFIX + _target);
}

int symlink::internal_read(const request::ptr &req)
{
  mutex::scoped_lock lock(_mutex, defer_lock);
  string output;

  req->init(base::HTTP_GET);
  req->set_url(get_url());

  req->run();
  
  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  output = req->get_output_string();

  if (strncmp(output.c_str(), CONTENT_PREFIX_CSTR, CONTENT_PREFIX_LEN) != 0) {
    S3_LOG(LOG_WARNING, "symlink::internal_read", "content prefix does not match: [%s]\n", output.c_str());
    return -EINVAL;
  }

  lock.lock();
  _target = output.c_str() + CONTENT_PREFIX_LEN;

  return 0;
}
