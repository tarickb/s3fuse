#include "object_builder.h"

using namespace s3;

object_builder::object_builder(const request::ptr &req, const string &path, object_type type_hint)
  : _req(req),
    _path(path),
    _type_hint(type_hint),
    _target_type(OT_INVALID)
{
  _req->init(HTTP_HEAD);

  _req->set_process_header_callback(bind(&object_builder::process_header, this, _1, _2));
}

object::ptr object_builder::build()
{
  if (_type_hint == OT_INVALID || _type_hint == OT_DIRECTORY)
    try_build(OT_DIRECTORY);

  if (!_obj)
    try_build(OT_FILE);

  return _obj;
}

void object_builder::try_build(object_type type)
{
  _target_type = type;
  _obj.reset();
  _headers.clear();

  _req->set_url(object::build_url(path, type));
  _req->run();

  if (_req->get_response_code() != HTTP_SC_OK)
    _obj.reset();

  if (_obj)
    _obj->build_finalize(_req);
}

void object_builder::process_header(const string &key, const string &value)
{
  if (!_obj) {
    // we use Content-Type to decide what sort of object this is
    if (key == "Content-Type") {
      if (_target_type == OT_DIRECTORY)
        _obj = object::create(_path, _target_type);
      else
        _obj = object::create(_path, (value == symlink::get_content_type()) ? OT_SYMLINK : OT_FILE);
    }

    if (_obj) // if we've just created an object, process saved headers
      for (header_list::const_iterator itor = _headers.begin(); itor != _headers.end(); ++itor)
        _obj->build_process_header(itor->first, itor->second);
    else // otherwise, save the header for when we create the object
      _headers.push_back(header_element(key, value));
  }

  if (_obj) // retest, since we might have created an object
    _obj->build_process_header(_req, key, value);
}
