/*
 * base/xml.cc
 * -------------------------------------------------------------------------
 * Implements s3::xml using libxml++.
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
#include <libxml/tree.h>

#include <stdexcept>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>

#include "logger.h"
#include "xml.h"

using boost::regex;
using std::runtime_error;
using std::string;
using xmlpp::DomParser;
using xmlpp::Element;
using xmlpp::NodeSet;
using xmlpp::TextNode;

using s3::base::xml;

namespace
{
  struct transform_pair
  {
    const regex expr;
    const string subst;
  };

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
}

void xml::init()
{
  xmlInitParser();
  LIBXML_TEST_VERSION;
}

xml::document xml::parse(const string &data)
{
  try {
    document doc(new DomParser());

    doc->parse_memory(transform(data));

    if (!doc)
      throw runtime_error("error while parsing xml.");

    if (!doc->get_document())
      throw runtime_error("parser does not contain a document.");

    if (!doc->get_document()->get_root_node())
      throw runtime_error("document does not contain a root node.");

    return doc;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::parse", "caught exception: %s\n", e.what());
  }

  return document();
}

int xml::find(const xml::document &doc, const char *xpath, string *element)
{
  try {
    NodeSet nodes;
    TextNode *text;

    if (!doc)
      throw runtime_error("got null document pointer.");

    nodes = doc->get_document()->get_root_node()->find(xpath);

    if (nodes.empty())
      throw runtime_error("no matching nodes.");

    text = dynamic_cast<Element *>(nodes[0])->get_child_text();

    if (!text)
      throw runtime_error("node does not contain a text child.");

    *element = text->get_content();
    return 0;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::find", "caught exception while finding [%s]: %s\n", xpath, e.what());
  }

  return -EIO;
}

int xml::find(const xml::document &doc, const char *xpath, xml::element_list *list)
{
  try {
    NodeSet nodes;

    if (!doc)
      throw runtime_error("got null document pointer.");

    nodes = doc->get_document()->get_root_node()->find(xpath);

    for (NodeSet::const_iterator itor = nodes.begin(); itor != nodes.end(); ++itor) {
      TextNode *text = dynamic_cast<Element *>(*itor)->get_child_text();

      if (text)
        list->push_back(text->get_content());
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
    DomParser dom;
    Element *root = NULL;

    dom.parse_memory(transform(data));

    if (!dom)
      return false;

    if (!dom.get_document())
      return false;

    root = dom.get_document()->get_root_node();

    if (!root)
      return false;

    return !root->find(xpath).empty();

  } catch (...) {}

  return false;
}
