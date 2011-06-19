/*
 * config.cc
 * -------------------------------------------------------------------------
 * Definitions for s3::config static members and init() method.
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

#include <errno.h>

#include <fstream>
#include <boost/lexical_cast.hpp>

#include "config.h"
#include "logger.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string DEFAULT_CONFIG_FILE = "/etc/s3fuse.conf";
}

#define CONFIG(type, name, def) type config::s_ ## name = (def);
#define CONFIG_REQUIRED(type, name, def) CONFIG(type, name, def)

#include "config.inc"

#undef CONFIG
#undef CONFIG_REQUIRED

int config::init(const string &file)
{
  ifstream ifs((file.empty() ? DEFAULT_CONFIG_FILE : file).c_str());
  int line_number = 0;

  if (ifs.fail()) {
    S3_LOG(LOG_ERR, "config::init", "cannot open config file.\n");
    return -EIO;
  }

  while (ifs.good()) {
    string line, key, value;
    size_t pos;

    getline(ifs, line);
    line_number++;
    pos = line.find('#');

    if (pos != string::npos)
      line = line.substr(0, pos);

    if (line.empty())
      continue;

    pos = line.find('=');

    if (pos == string::npos) {
      S3_LOG(LOG_ERR, "config::init", "error at line %i: missing '='.\n", line_number);
      return -EIO;
    }

    key = line.substr(0, pos);
    value = line.substr(pos + 1);

    #define CONFIG(type, name, def) \
      if (key == #name) { \
        s_ ## name = lexical_cast<type>(value); \
        continue; \
      }

    #define CONFIG_REQUIRED(type, name, def) CONFIG(type, name, def)

    #include "config.inc"

    #undef CONFIG
    #undef CONFIG_REQUIRED

    S3_LOG(LOG_ERR, "config::init", "error at line %i: unknown directive '%s'\n", line_number, key.c_str());
    return -EIO;
  }

  #define CONFIG(type, name, def)

  #define CONFIG_REQUIRED(type, name, def) \
    if (s_ ## name == (def)) { \
      S3_LOG(LOG_ERR, "config::init", "required key '%s' not defined.\n", #name); \
      return -EIO; \
    }

  #include "config.inc"

  #undef CONFIG
  #undef CONFIG_REQUIRED

  return 0;
}
