/*
 * fs/object_acls.cc
 * -------------------------------------------------------------------------
 * object acl lookup implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2014, Hiroyuki Kakine.
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

#include "base/logger.h"
#include "base/config.h"
#include "base/paths.h"
#include "fs/object_acls.h"

using boost::is_any_of;
using boost::token_compress_on;
using std::ifstream;
using std::map;
using std::pair;
using std::string;
using std::vector;

using s3::base::config;
using s3::base::paths;
using s3::fs::object_acls;

namespace
{
  typedef struct object_acl_value object_acl_value_t;
  typedef map<string, object_acl_value> path_map;

  struct object_acl_value {
      object_acl_value(string a, path_map p) {
          acl = a;
          child_path_map = p;
      }
      string acl;
      path_map child_path_map;
  };

  path_map root;
  string nomatch = "";

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
      if (fields.size() < 2) {
        continue;
      }
      path_map *m = &root;
      size_t start_idx = 0;
      size_t end_idx = 0;
      while (1) {
        end_idx = fields[0].find("/", start_idx);
        if (end_idx == string::npos) {
          break;
        }
        string sub_path = fields[0].substr(start_idx, end_idx - start_idx);
        start_idx = end_idx + 1;
        map<string, object_acl_value_t>::iterator itr = (*m).find(sub_path);
        if (itr == (*m).end()) {
          // new 
          path_map child_path_map;
          object_acl_value_t value = object_acl_value_t(nomatch, child_path_map);
          (*m).insert(make_pair(sub_path, value));
          m = &child_path_map;
        } else {
          // already exist
          object_acl_value value = (*itr).second;
          m = &(value.child_path_map);
        }
      }
      string sub_path = fields[0].substr(start_idx);
      map<string, object_acl_value>::iterator itr = (*m).find(sub_path);
      if (itr == (*m).end()) {
        // new 
        path_map child_path_map;
        object_acl_value value = object_acl_value(fields[1], child_path_map);
        (*m).insert(make_pair(sub_path, value)); 
      } else {
        // already exist
        object_acl_value value = (*itr).second;
        value.acl = fields[1];
      }
    }
  }
}

void object_acls::init()
{
  load_from_file(config::get_object_acls());
  load_from_file("~/.object.acls");
}

const string & object_acls::get_acl(string path)
{
  string *last_match_acl = &nomatch;
  path_map *m = &root;
  unsigned int start_idx = 0;
  unsigned int end_idx = 0;
  while ((end_idx = path.find("/", start_idx + 1)) != string::npos) {
      string sub_path = path.substr(start_idx, end_idx);
      start_idx = end_idx;
      map<string, object_acl_value>::iterator itr = (*m).find(sub_path);
      if (itr == (*m).end()) {
          return (*last_match_acl);
      } else {
          object_acl_value value = (*itr).second;
          last_match_acl = &(value.acl);
          m = &(value.child_path_map);
          if ((*m).empty())  {
              return (*last_match_acl);
          }
      }
  }
  return nomatch;
}
