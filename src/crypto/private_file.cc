/*
 * crypto/private_file.cc
 * -------------------------------------------------------------------------
 * "Private" file management (implementation).
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>

#include "crypto/private_file.h"

using std::ifstream;
using std::ios;
using std::ofstream;
using std::runtime_error;
using std::string;

using s3::crypto::private_file;

void private_file::open(const string &file, ofstream *f, open_mode mode)
{
  f->open(file.c_str(), ios::out | static_cast<ios::openmode>(mode == OM_TRUNCATE ? ios::trunc : 0));

  if (!f->good())
    throw runtime_error("unable to open/create private file.");

  if (chmod(file.c_str(), S_IRUSR | S_IWUSR))
    throw runtime_error("failed to set permissions on private file.");
}

void private_file::open(const string &file, ifstream *f)
{
  struct stat s;

  f->open(file.c_str(), ios::in);

  if (!f->good())
    throw runtime_error("unable to open private file.");

  if (stat(file.c_str(), &s))
    throw runtime_error("unable to stat private file.");

  if ((s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRUSR | S_IWUSR))
    throw runtime_error("private file must be readable/writeable only by owner.");
}
