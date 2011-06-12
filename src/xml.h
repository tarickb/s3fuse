#ifndef S3_XML_H
#define S3_XML_H

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

    static void init();

    static document parse(const std::string &data);

    static int find(const document &doc, const char *xpath, std::string *element);
    static int find(const document &doc, const char *xpath, element_list *elements);
  };
}

#endif
