/*
 * fs/mime_types.cc
 * -------------------------------------------------------------------------
 * MIME type lookup implementation.
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

#include <fstream>
#include <map>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "base/paths.h"
#include "fs/mime_types.h"

using boost::is_any_of;
using boost::token_compress_on;
using boost::algorithm::to_lower;
using std::ifstream;
using std::map;
using std::string;
using std::vector;

using s3::base::paths;
using s3::fs::mime_types;

namespace
{
  typedef map<string, string> type_map;

  const char *MAP_FILES[] = {
    "/etc/httpd/mime.types",
    "/private/etc/apache2/mime.types", // this just happens to be where I found the mime map on my OS X system
    "/etc/mime.types",
    "~/.mime.types" };

  const size_t MAP_FILE_COUNT = sizeof(MAP_FILES) / sizeof(MAP_FILES[0]);

  type_map s_map;

  void load_from_file(const string &path)
  {
    string line;
    vector<string> fields;
    ifstream f(paths::transform(path).c_str(), ifstream::in);

    while (f.good()) {
      size_t pos;

      getline(f, line);

      pos = line.find('#');

      if (pos == 0)
        continue;
      else if (pos != string::npos)
        line = line.substr(0, pos);

      split(fields, line, is_any_of(string(" \t")), token_compress_on);

      for (size_t i = 1; i < fields.size(); i++)
        s_map[fields[i]] = fields[0];
    }
  }
}

void mime_types::init()
{
  for (size_t i = 0; i < MAP_FILE_COUNT; i++)
    load_from_file(MAP_FILES[i]);
}

string mime_types::get_type_by_extension(string ext)
{
  to_lower(ext);

  const type_map::const_iterator &iter = s_map.find(ext);
  return (iter == s_map.end()) ? "" : iter->second;
}
