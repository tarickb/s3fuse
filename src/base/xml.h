/*
 * base/xml.h
 * -------------------------------------------------------------------------
 * Simplified XML parser interface.
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

#ifndef S3_BASE_XML_H
#define S3_BASE_XML_H

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace s3 {
namespace base {
class XmlDocument {
 public:
  static constexpr char MAP_NAME_KEY[] = "__element_name__";

  static void Init();
  static std::unique_ptr<XmlDocument> Parse(std::string data);

  inline static std::unique_ptr<XmlDocument> Parse(const char *in, size_t len) {
    std::string s;
    s.assign(in, len);
    return Parse(s);
  }

  inline static std::unique_ptr<XmlDocument> Parse(
      const std::vector<char> &in) {
    return Parse(&in[0], in.size());
  }

  virtual ~XmlDocument() = default;

  virtual int Find(const std::string &xpath, std::string *element) = 0;
  virtual int Find(const std::string &xpath,
                   std::list<std::string> *elements) = 0;
  virtual int Find(const std::string &xpath,
                   std::list<std::map<std::string, std::string>> *list) = 0;

  virtual bool Match(const std::string &xpath) = 0;
};
}  // namespace base
}  // namespace s3

#endif
