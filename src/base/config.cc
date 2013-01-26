/*
 * base/config.cc
 * -------------------------------------------------------------------------
 * Definitions for s3::config static members and init() method.
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

#include <fstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/paths.h"

using boost::lexical_cast;
using std::ifstream;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::base::paths;

namespace
{
  const string DEFAULT_CONFIG_FILES[] = {
    "~/.s3fuse/s3fuse.conf",
    SYSCONFDIR "/s3fuse.conf" };

  const int DEFAULT_CONFIG_FILE_COUNT = sizeof(DEFAULT_CONFIG_FILES) / sizeof(DEFAULT_CONFIG_FILES[0]);

  template <typename T>
  class option_parser_worker
  {
  public:
    static void parse(const string &str, T *out)
    {
      *out = lexical_cast<T>(str);
    }
  };

  template <>
  class option_parser_worker<bool>
  {
  public:
    static void parse(const string &_str, bool *out)
    {
      const char *str = _str.c_str();

      if (!strcasecmp(str, "yes") || !strcasecmp(str, "true")  || !strcasecmp(str, "1") || !strcasecmp(str, "on") ) {
        *out = true;
        return;
      }

      if (!strcasecmp(str, "no")  || !strcasecmp(str, "false") || !strcasecmp(str, "0") || !strcasecmp(str, "off")) {
        *out = false;
        return;
      }

      throw runtime_error("cannot parse.");
    }
  };

  template <typename T>
  class option_parser
  {
  public:
    static void parse(int line_number, const char *key, const char *type, const string &str, T *out)
    {
      try {
        option_parser_worker<T>::parse(str, out);

      } catch (...) {
        S3_LOG(
          LOG_ERR, 
          "config::init", "error at line %i: cannot parse [%s] for key [%s] of type %s.\n", 
          line_number,
          str.c_str(),
          key,
          type);

        throw;
      }
    }
  };
}

#define CONFIG(type, name, def, desc) type config::s_ ## name = (def);
#define CONFIG_REQUIRED(type, name, def, desc) CONFIG(type, name, def, desc)
#define CONFIG_CONSTRAINT(x, y)
#define CONFIG_KEY(x)

#include "base/config.inc"

#undef CONFIG
#undef CONFIG_REQUIRED
#undef CONFIG_CONSTRAINT
#undef CONFIG_KEY

void config::init(const string &file)
{
  ifstream ifs;
  int line_number = 0;

  if (file.empty()) {
    for (int i = 0; i < DEFAULT_CONFIG_FILE_COUNT; i++) {
      ifs.open(paths::transform(DEFAULT_CONFIG_FILES[i]).c_str());

      if (ifs.good())
        break;
    }

    if (ifs.fail()) {
      for (int i = 0; i < DEFAULT_CONFIG_FILE_COUNT; i++)
        S3_LOG(LOG_ERR, "config::init", "unable to open configuration in [%s]\n", DEFAULT_CONFIG_FILES[i].c_str());

      throw runtime_error("cannot open any default config files");
    }
  } else {
    ifs.open(paths::transform(file).c_str());

    if (ifs.fail()) {
      S3_LOG(LOG_ERR, "config::init", "cannot open file [%s].\n", file.c_str());
      throw runtime_error("cannot open specified config file");
    }
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
      throw runtime_error("malformed config file");
    }

    key = line.substr(0, pos);
    value = line.substr(pos + 1);

    #define CONFIG(type, name, def, desc) \
      if (key == #name) { \
        option_parser<type>::parse( \
          line_number, \
          #name, \
          #type, \
          value, \
          &s_ ## name); \
        \
        continue; \
      }

    #define CONFIG_REQUIRED(type, name, def, desc) CONFIG(type, name, def, desc)

    #define CONFIG_CONSTRAINT(x, y)
    #define CONFIG_KEY(x)

    #include "base/config.inc"

    #undef CONFIG
    #undef CONFIG_REQUIRED
    #undef CONFIG_CONSTRAINT
    #undef CONFIG_KEY

    S3_LOG(LOG_ERR, "config::init", "error at line %i: unknown directive '%s'\n", line_number, key.c_str());
    throw runtime_error("malformed config file");
  }

  #define CONFIG(type, name, def, desc)

  #define CONFIG_REQUIRED(type, name, def, desc) \
    if (s_ ## name == (def)) { \
      S3_LOG(LOG_ERR, "config::init", "required key '%s' not defined.\n", #name); \
      throw runtime_error("malformed config file"); \
    }

  #define CONFIG_CONSTRAINT(test, message) \
    if (!(test)) { \
      S3_LOG(LOG_ERR, "config::init", "%s\n", message); \
      throw runtime_error("malformed config file"); \
    }

  #define CONFIG_KEY(key) s_ ## key

  #include "base/config.inc"

  #undef CONFIG
  #undef CONFIG_REQUIRED
  #undef CONFIG_CONSTRAINT
  #undef CONFIG_KEY
}
