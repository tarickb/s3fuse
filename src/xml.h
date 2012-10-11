/*
 * xml.h
 * -------------------------------------------------------------------------
 * Simplified XML parser interface.
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

#ifndef S3_XML_H
#define S3_XML_H

#include <list>
#include <string>
#include <boost/smart_ptr.hpp>

namespace xmlpp
{
  class DomParser;
}

namespace s3
{
  class xml
  {
  public:
    typedef boost::shared_ptr<xmlpp::DomParser> document;
    typedef std::list<std::string> element_list;

    static void init(const std::string &ns);

    static document parse(const std::string &data);

    static int find(const document &doc, const char *xpath, std::string *element);
    static int find(const document &doc, const char *xpath, element_list *elements);
  };
}

#endif
