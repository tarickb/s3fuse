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
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace xmlpp
{
  class DomParser;
}

namespace s3
{
  namespace base
  {
    class xml
    {
    public:
      typedef boost::shared_ptr<xmlpp::DomParser> document;
      typedef std::list<std::string> element_list;

      static void init();

      static document parse(const std::string &data);

      static int find(const document &doc, const char *xpath, std::string *element);
      static int find(const document &doc, const char *xpath, element_list *elements);

      static bool match(const std::string &data, const char *xpath);

      inline static bool match(const std::vector<char> &in, const char *xpath)
      {
        return match(&in[0], in.size(), xpath);
      }

      inline static bool match(const char *in, size_t len, const char *xpath)
      {
        std::string s;

        s.assign(in, len);

        return match(s, xpath);
      }
    };
  }
}

#endif
