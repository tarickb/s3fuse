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

#include "fs/mime_types.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <istream>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

#include "base/paths.h"

namespace s3 {
namespace fs {

namespace {
const char *MAP_FILES[] = {
    "/etc/httpd/mime.types",
    "/private/etc/apache2/mime.types",  // this just happens to be where I found
                                        // the mime map on my OS X system
    "/etc/mime.types", "~/.mime.types"};

std::map<std::string, std::string> s_map;

void LoadFromFile(const std::string &path) {
  std::ifstream f(base::Paths::Transform(path).c_str(), std::ifstream::in);

  while (f.good()) {
    std::string line;
    std::getline(f, line);

    size_t pos = line.find('#');
    if (pos == 0)
      continue;
    else if (pos != std::string::npos)
      line = line.substr(0, pos);

    std::istringstream line_stream(line);
    std::vector<std::string> fields{
        std::istream_iterator<std::string>(line_stream),
        std::istream_iterator<std::string>()};

    for (size_t i = 1; i < fields.size(); i++) s_map[fields[i]] = fields[0];
  }
}
}  // namespace

void MimeTypes::Init() {
  for (const auto *file : MAP_FILES) LoadFromFile(file);
}

std::string MimeTypes::GetTypeByExtension(std::string ext) {
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](char c) { return std::tolower(c); });
  const auto &iter = s_map.find(ext);
  return (iter == s_map.end()) ? "" : iter->second;
}

}  // namespace fs
}  // namespace s3
