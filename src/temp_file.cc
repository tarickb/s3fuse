/*
 * temp_file.cc
 * -------------------------------------------------------------------------
 * Temporary file wrapper definition.
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

#include <stdlib.h>

#include <stdexcept>

#include "temp_file.h"

using namespace std;

using namespace s3;

temp_file::temp_file()
{
  char name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(name);

  if (_fd == -1)
    throw runtime_error("unable to create temporary file.");

  unlink(name);
}

temp_file::~temp_file()
{
  close(_fd);
}
