/*
 * xml.cc
 * -------------------------------------------------------------------------
 * Implements s3::xml using libxml++.
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

#include <errno.h>
#include <libxml/tree.h>

#include <stdexcept>
#include <libxml++/libxml++.h>

#include "logger.h"
#include "xml.h"

using namespace std;
using namespace xmlpp;

using namespace s3;

namespace
{
  const char *AWS_S3_NS = "http://s3.amazonaws.com/doc/2006-03-01/";

  Node::PrefixNsMap s_ns_map;
}

void xml::init()
{
  xmlInitParser();
  LIBXML_TEST_VERSION;

  s_ns_map["s3"] = AWS_S3_NS;
}

xml::document xml::parse(const string &data)
{
  try {
    document doc(new DomParser());

    doc->parse_memory(data);

    if (!doc)
      throw runtime_error("error while parsing xml.");

    if (!doc->get_document())
      throw runtime_error("parser does not contain a document.");

    if (!doc->get_document()->get_root_node())
      throw runtime_error("document does not contain a root node.");

    return doc;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::parse", "caught exception: %s\n", e.what());

  } catch (...) {
    S3_LOG(LOG_WARNING, "xml::parse", "caught unknown exception.");
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

    nodes = doc->get_document()->get_root_node()->find(xpath, s_ns_map);

    if (nodes.empty())
      throw runtime_error("no matching nodes.");

    text = dynamic_cast<Element *>(nodes[0])->get_child_text();

    if (!text)
      throw runtime_error("node does not contain a text child.");

    *element = text->get_content();
    return 0;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::find", "caught exception while finding [%s]: %s\n", xpath, e.what());

  } catch (...) {
    S3_LOG(LOG_WARNING, "xml::find", "caught unknown exception while finding [%s].\n", xpath);
  }

  return -EIO;
}

int xml::find(const xml::document &doc, const char *xpath, xml::element_list *list)
{
  try {
    NodeSet nodes;

    if (!doc)
      throw runtime_error("got null document pointer.");

    nodes = doc->get_document()->get_root_node()->find(xpath, s_ns_map);

    for (NodeSet::const_iterator itor = nodes.begin(); itor != nodes.end(); ++itor) {
      TextNode *text = dynamic_cast<Element *>(*itor)->get_child_text();

      if (text)
        list->push_back(text->get_content());
    }

    return 0;

  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "xml::find", "caught exception while finding [%s]: %s\n", xpath, e.what());

  } catch (...) {
    S3_LOG(LOG_WARNING, "xml::find", "caught unknown exception while finding [%s].\n", xpath);
  }

  return -EIO;
}
