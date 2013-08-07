/*
 * base/xml.cc
 * -------------------------------------------------------------------------
 * Implements s3::base::xml using libxml2.
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

#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <stdexcept>
#include <boost/regex.hpp>
#include <boost/utility.hpp>

#include "logger.h"
#include "xml.h"

using boost::regex;
using std::runtime_error;
using std::string;

using s3::base::xml;

namespace
{
  struct transform_pair
  {
    const regex expr;
    const string subst;
  };

  // strip out namespace declarations
  transform_pair TRANSFORMS[] = {
    { regex(" xmlns(:\\w*)?=\"[^\"]*\""), "" },
    { regex(" xmlns(:\\w*)?='[^']*'"), "" },
    { regex("<\\w*:"), "<" },
    { regex("</\\w*:"), "</" } };

  const int TRANSFORM_COUNT = sizeof(TRANSFORMS) / sizeof(TRANSFORMS[0]);

  string transform(string in)
  {
    for (int i = 0; i < TRANSFORM_COUNT; i++)
      in = regex_replace(in, TRANSFORMS[i].expr, TRANSFORMS[i].subst);

    return in;
  }

  template <class T, void (*DTOR)(T *)>
  class libxml_ptr : boost::noncopyable
  {
  public:
    libxml_ptr()
      : _ptr(NULL)
    {
    }

    explicit libxml_ptr(T *ptr)
      : _ptr(ptr)
    {
    }

    virtual ~libxml_ptr()
    {
      if (_ptr)
        DTOR(_ptr);
    }

    inline operator T * ()
    {
      return _ptr;
    }

    inline void operator =(T *ptr)
    {
      if (_ptr)
        DTOR(_ptr);

      _ptr = ptr;
    }

    inline T * operator->()
    {
      return _ptr;
    }

    inline bool is_null()
    {
      return (_ptr == NULL);
    }

  private:
    T *_ptr;
  };

  typedef libxml_ptr<xmlDoc, xmlFreeDoc> doc_wrapper;
  typedef libxml_ptr<xmlXPathContext, xmlXPathFreeContext> xpath_context_wrapper;
  typedef libxml_ptr<xmlXPathObject, xmlXPathFreeObject> xpath_object_wrapper;

  xmlXPathObject * xpath_find(doc_wrapper *doc, const char *xpath)
  {
    xpath_context_wrapper context;
    xmlNodePtr root;

    root = xmlDocGetRootElement(*doc);

    if (!root)
      throw runtime_error("cannot find root element");

    context = xmlXPathNewContext(*doc);

    return xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(xpath), context);
  }
}

class xml::document : public doc_wrapper
{
public:
  document(xmlDoc *doc)
    : doc_wrapper(doc)
  {
  }
};

void xml::init()
{
  xmlInitParser();
  LIBXML_TEST_VERSION;
}

xml::document_ptr xml::parse(const string &data_)
{
  try {
    string data = transform(data_);
    document_ptr doc(new document(xmlParseMemory(data.c_str(), data.size())));

    if (!doc)
      throw runtime_error("error while parsing xml.");

    if (!xmlDocGetRootElement(*doc))
      throw runtime_error("document does not contain a root node.");

    return doc;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::parse", "caught exception: %s\n", e.what());
  }

  return document_ptr();
}

int xml::find(const xml::document_ptr &doc, const char *xpath, string *element)
{
  try {
    xpath_object_wrapper result;
    xmlChar *text;

    if (!doc)
      throw runtime_error("cannot search empty document");

    result = xpath_find(doc.get(), xpath);

    if (result.is_null())
      throw runtime_error("invalid xpath expression");

    if (xmlXPathNodeSetIsEmpty(result->nodesetval))
      throw runtime_error("no matching nodes.");

    text = xmlXPathCastNodeToString(result->nodesetval->nodeTab[0]);

    if (text) {
      *element = reinterpret_cast<const char *>(text);
      xmlFree(text);
    }

    return 0;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::find", "caught exception while finding [%s]: %s\n", xpath, e.what());
  }

  return -EIO;
}

int xml::find(const xml::document_ptr &doc, const char *xpath, xml::element_list *list)
{
  try {
    xpath_object_wrapper result;

    if (!doc)
      throw runtime_error("cannot search empty document");

    result = xpath_find(doc.get(), xpath);

    if (result.is_null())
      throw runtime_error("invalid xpath expression");

    for (int i = 0; i < result->nodesetval->nodeNr; i++) {
      xmlChar *text;

      text = xmlXPathCastNodeToString(result->nodesetval->nodeTab[i]);

      if (text) {
        list->push_back(string(reinterpret_cast<const char *>(text)));
        xmlFree(text);
      }
    }

    return 0;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::find", "caught exception while finding [%s]: %s\n", xpath, e.what());
  }

  return -EIO;
}

bool xml::match(const string &data, const char *xpath)
{
  try {
    document_ptr doc = parse(data);
    xpath_object_wrapper result;

    if (!doc)
      throw runtime_error("failed to parse xml");

    result = xpath_find(doc.get(), xpath);

    if (result.is_null())
      throw runtime_error("invalid xpath expression");

    return !xmlXPathNodeSetIsEmpty(result->nodesetval);

  } catch (...) {}

  return false;
}
