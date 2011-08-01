#ifndef S3_OBJECT_BUILDER_H
#define S3_OBJECT_BUILDER_H

#include <list>
#include <string>
#include <utility>
#include <boost/smart_ptr.hpp>

#include "object.h"

namespace s3
{
  class request;

  class object_builder
  {
  public:
    object_builder(const boost::shared_ptr<request> &req, const std::string &path, object_type type_hint = OT_INVALID);

    object::ptr build();

  private:
    typedef std::pair<std::string, std::string> header_element;
    typedef std::list<header_element> header_list;

    void try_build(object_type type);
    void process_header(const std::string &key, const std::string &value);

    boost::shared_ptr<request> _req;
    std::string _path;
    boost::shared_ptr<object> _obj;
    header_list _headers;
    object_type _type_hint, _target_type;
  };
}

#endif
