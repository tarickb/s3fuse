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

#include <algorithm>
#include <cctype>
#include <fstream>
#include <istream>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

#include "base/paths.h"
#include "fs/mime_types.h"

using std::ifstream;
using std::istream_iterator;
using std::istringstream;
using std::map;
using std::string;
using std::tolower;
using std::transform;
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
    ifstream f(paths::transform(path).c_str(), ifstream::in);

    while (f.good()) {
      string line;
      getline(f, line);

      size_t pos= line.find('#');
      if (pos == 0)
        continue;
      else if (pos != string::npos)
        line = line.substr(0, pos);

      istringstream line_stream(line);
      vector<string> fields{istream_iterator<string>(line_stream), istream_iterator<string>()};

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
  transform(ext.begin(), ext.end(), ext.begin(), [](char c){ return tolower(c); });

  const type_map::const_iterator &iter = s_map.find(ext);
  return (iter == s_map.end()) ? "" : iter->second;
}
