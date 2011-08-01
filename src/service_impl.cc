/*
 * service_impl.cc
 * -------------------------------------------------------------------------
 * Definitions for base service implementation static methods.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>

#include "service_impl.h"

using namespace std;

using namespace s3;

namespace
{
  const string META_PREFIX = "meta-";
}

const string & service_impl::get_si_meta_prefix()
{
  return META_PREFIX;
}

void service_impl::open_private_file(const string &file, ofstream *f)
{
  f->open(file.c_str(), ios::out | ios::trunc);

  if (!f->good())
    throw runtime_error("unable to open/create private file.");

  if (chmod(file.c_str(), S_IRUSR | S_IWUSR))
    throw runtime_error("failed to set permissions on private file.");
}

void service_impl::open_private_file(const string &file, ifstream *f)
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
