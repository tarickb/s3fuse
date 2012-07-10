#include <cctype>
#include <stdexcept>

#include "util.h"
#include "xattr.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string  XATTR_HEADER_PREFIX      = "s3fuse_xattr_";

  const char   *XATTR_HEADER_PREFIX_CSTR = XATTR_HEADER_PREFIX.c_str();
  const size_t  XATTR_HEADER_PREFIX_LEN  = XATTR_HEADER_PREFIX.size();

  inline bool is_valid_key(const string &value)
  {
    for (size_t i = 0; i < value.length(); i++)
      if (value[i] != '-' && value[i] != '_' && !isdigit(value[i]) && !islower(value[i]))
        return false;

    return true;
  }
}

xattr::ptr xattr::from_string(const string &key, const string &value)
{
  ptr ret(new xattr(key, false));

  // we don't intend for xattr objects constructed by this method to be used as real attributes
  ret->_is_serializable = false;

  ret->_value.resize(value.size() + 1);
  memcpy(&ret->_value[0], reinterpret_cast<const uint8_t *>(value.c_str()), value.size() + 1);

  return ret;
}

xattr::ptr xattr::from_header(const string &header_key, const string &header_value)
{
  ptr ret;

  if (strncmp(header_key.c_str(), XATTR_HEADER_PREFIX_CSTR, XATTR_HEADER_PREFIX_LEN) == 0) {
    vector<uint8_t> dec_key;
    size_t separator = header_value.find(' ');

    if (separator == string::npos)
      throw runtime_error("header string is malformed.");

    util::base64_decode(header_value.substr(0, separator), &dec_key);

    ret.reset(new xattr(reinterpret_cast<char *>(&dec_key[0]), true));

    util::base64_decode(header_value.substr(separator + 1), &ret->_value);

  } else {
    ret = from_string(header_key, header_value);

    // from_string() sets _is_serializable to false because it doesn't know if the header/value 
    // combination comes from a valid S3 object.  this method does, and as such can safely set
    // _is_serializable to true.

    ret->_is_serializable = true;

    // we know the value doesn't need encoding because it came to us as a valid HTTP string.

    ret->_value_needs_encoding = false;
  }

  return ret;
}

xattr::ptr xattr::create(const string &key)
{
  ptr ret(new xattr(key, is_valid_key(key)));

  return ret;
}

xattr::xattr(const string &key, bool key_needs_encoding)
  : _key(key),
    _key_needs_encoding(key_needs_encoding),
    _value_needs_encoding(true),
    _is_serializable(true)
{
}

void xattr::set_value(const char *value, size_t size)
{
  _value.resize(size);
  memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

  _value_needs_encoding = (value[size] != '\0' || strlen(value) != size || !util::is_valid_http_string(string(value)));
}

int xattr::get_value(char *buffer, size_t max_size)
{
  if (buffer == NULL || _value.size() > max_size)
    return _value.size();

  memcpy(reinterpret_cast<uint8_t *>(buffer), &_value[0], _value.size());

  return 0;
}

void xattr::to_header(string *header, string *value)
{
  if (!_is_serializable)
    throw runtime_error("this extended attribute cannot be serialized.");

  if (_key_needs_encoding || _value_needs_encoding) {
    *header = XATTR_HEADER_PREFIX + util::compute_md5(_key);
    *value = util::base64_encode(reinterpret_cast<const uint8_t *>(_key.c_str()), _key.size() + 1) + " " + util::base64_encode(&_value[0], _value.size());
  } else {
    *header = _key;
    *value = reinterpret_cast<char *>(&_value[0]);
  }
}

