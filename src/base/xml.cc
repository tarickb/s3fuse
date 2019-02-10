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

#include "base/xml.h"

#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <regex>
#include <stdexcept>

#include "base/logger.h"

namespace s3 {
namespace base {

namespace {
struct TransformPair {
  const std::regex expr;
  const std::string subst;
};

// strip out namespace declarations
TransformPair TRANSFORMS[] = {{std::regex(" xmlns(:\\w*)?=\"[^\"]*\""), ""},
                              {std::regex(" xmlns(:\\w*)?='[^']*'"), ""},
                              {std::regex("<\\w*:"), "<"},
                              {std::regex("</\\w*:"), "</"}};

std::string Transform(std::string in) {
  for (const auto &t : TRANSFORMS) in = std::regex_replace(in, t.expr, t.subst);
  return in;
}

void FreeXmlChar(xmlChar *p) { xmlFree(p); }

using XmlDocPtr = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
using XPathContextPtr =
    std::unique_ptr<xmlXPathContext, decltype(&xmlXPathFreeContext)>;
using XPathObjectPtr =
    std::unique_ptr<xmlXPathObject, decltype(&xmlXPathFreeObject)>;
using XmlCharPtr = std::unique_ptr<xmlChar, decltype(&FreeXmlChar)>;

class XmlDocumentImpl : public XmlDocument {
 public:
  explicit XmlDocumentImpl(XmlDocPtr doc)
      : doc_(std::move(doc)),
        xpath_context_(xmlXPathNewContext(doc_.get()), &xmlXPathFreeContext) {}

  ~XmlDocumentImpl() override = default;

  int Find(const char *xpath, std::string *element) {
    try {
      auto result = EvalXPath(xpath);
      if (!result) throw std::runtime_error("invalid xpath expression");

      if (xmlXPathNodeSetIsEmpty(result->nodesetval))
        throw std::runtime_error("no matching nodes.");

      auto text = XPathNodeToString(result->nodesetval->nodeTab[0]);
      if (text) *element = reinterpret_cast<const char *>(text.get());

      return 0;
    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "XmlDocument::Find",
             "caught exception while finding [%s]: %s\n", xpath, e.what());
    }

    return -EIO;
  }

  int Find(const char *xpath, std::list<std::string> *elements) {
    try {
      auto result = EvalXPath(xpath);
      if (!result) throw std::runtime_error("invalid xpath expression");

      for (int i = 0; i < result->nodesetval->nodeNr; i++) {
        auto text = XPathNodeToString(result->nodesetval->nodeTab[i]);
        if (text)
          elements->push_back(reinterpret_cast<const char *>(text.get()));
      }

      return 0;
    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "XmlDocument::Find",
             "caught exception while finding [%s]: %s\n", xpath, e.what());
    }

    return -EIO;
  }

  int Find(const char *xpath,
           std::list<std::map<std::string, std::string>> *list) {
    try {
      auto result = EvalXPath(xpath);
      if (!result) throw std::runtime_error("invalid xpath expression");

      for (int i = 0; i < result->nodesetval->nodeNr; i++) {
        xmlNodePtr node = result->nodesetval->nodeTab[i];
        std::map<std::string, std::string> elements;

        for (xmlNodePtr child = node->children; child != nullptr;
             child = child->next) {
          if (child->type != XML_ELEMENT_NODE) continue;

          auto text = XPathNodeToString(child);
          if (text) {
            const auto *name = reinterpret_cast<const char *>(child->name);
            const auto *value = reinterpret_cast<const char *>(text.get());
            elements[name] = value;
          }
        }

        if (elements.find(MAP_NAME_KEY) == elements.end())
          elements[MAP_NAME_KEY] = reinterpret_cast<const char *>(node->name);
        else
          S3_LOG(LOG_WARNING, "XmlDocument::Find",
                 "unable to insert element name key.\n");

        list->push_back(elements);
      }

      return 0;
    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "XmlDocument::Find",
             "caught exception while finding [%s]: %s\n", xpath, e.what());
    }

    return -EIO;
  }

  bool Match(const char *xpath) {
    try {
      auto result = EvalXPath(xpath);
      if (!result) throw std::runtime_error("invalid xpath expression");

      return !xmlXPathNodeSetIsEmpty(result->nodesetval);
    } catch (...) {
    }

    return false;
  }

 private:
  XPathObjectPtr EvalXPath(const char *xpath) {
    auto *result = xmlXPathEvalExpression(
        reinterpret_cast<const xmlChar *>(xpath), xpath_context_.get());
    return {result, &xmlXPathFreeObject};
  }

  XmlCharPtr XPathNodeToString(xmlNodePtr node) {
    return {xmlXPathCastNodeToString(node), FreeXmlChar};
  }

  XmlDocPtr doc_;
  XPathContextPtr xpath_context_;
};
}  // namespace

constexpr char XmlDocument::MAP_NAME_KEY[];

void XmlDocument::Init() {
  xmlInitParser();
  LIBXML_TEST_VERSION;
}

std::unique_ptr<XmlDocument> XmlDocument::Parse(std::string data) {
  try {
    data = Transform(data);
    XmlDocPtr doc(xmlParseMemory(data.c_str(), data.size()), &xmlFreeDoc);
    if (!doc) throw std::runtime_error("error while parsing xml.");
    if (!xmlDocGetRootElement(doc.get()))
      throw std::runtime_error("document does not contain a root node.");
    return std::unique_ptr<XmlDocumentImpl>(
        new XmlDocumentImpl(std::move(doc)));
  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "XmlDocument::Parse", "caught exception: %s\n",
           e.what());
  }
  return {};
}

}  // namespace base
}  // namespace s3
