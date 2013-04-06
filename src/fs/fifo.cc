/*
 * fs/fifo.cc
 * -------------------------------------------------------------------------
 * Fake FIFO class implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#include <fcntl.h>

#include "base/logger.h"
#include "base/request.h"
#include "fs/fifo.h"

using std::string;

using s3::base::request;
using s3::fs::fifo;
using s3::fs::object;

namespace
{
  const string CONTENT_TYPE = "binary/s3fuse-fifo_0100"; // version 1.0

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new fifo(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 100);
}

fifo::fifo(const string &path)
  : file(path)
{
  set_content_type(CONTENT_TYPE);
  set_type(S_IFIFO);
}

fifo::~fifo()
{
}

int fifo::open_local_store(int *fd)
{
  char temp_name[] = "/tmp/s3fuse.fifo-XXXXXX";

  if (mktemp(temp_name) == NULL)
    return -errno;

  if (mkfifo(temp_name, 0600) != 0)
    return -errno;

  *fd = ::open(temp_name, O_RDWR);
  unlink(temp_name);

  if (*fd == -1)
    return -errno;

  S3_LOG(LOG_DEBUG, "fifo::open_local_store", "opening [%s] in [%s].\n", get_path().c_str(), temp_name);
  return 0;
}
