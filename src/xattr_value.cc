#include <errno.h>
#include <string.h>

#include <cctype>
#include <stdexcept>

#include "util.h"
#include "xattr_value.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const size_t  MAX_STRING_SCAN_LEN      = 128;
  const string  XATTR_HEADER_PREFIX      = "s3fuse_xattr_";

  const char   *XATTR_HEADER_PREFIX_CSTR = XATTR_HEADER_PREFIX.c_str();
  const size_t  XATTR_HEADER_PREFIX_LEN  = XATTR_HEADER_PREFIX.size();

  inline bool is_key_valid(const string &key)
  {
    for (size_t i = 0; i < key.length(); i++)
      if (key[i] != '.' && key[i] != '-' && key[i] != '_' && !isdigit(key[i]) && !islower(key[i]))
        return false;

    return true;
  }

  inline bool is_value_valid(const char *value, size_t size)
  {
    if (size > MAX_STRING_SCAN_LEN)
      return false;

    for (size_t i = 0; i < size; i++)
      if (value[i] != '/' && value[i] != '.' && value[i] != '-' && value[i] != '*' && value[i] != '_' && !isalnum(value[i]))
        return false;

    return true;
  }
}

xattr::ptr xattr_value::from_header(const string &header_key, const string &header_value)
{
  ptr ret;
  xattr_value *val = NULL;

  if (strncmp(header_key.c_str(), XATTR_HEADER_PREFIX_CSTR, XATTR_HEADER_PREFIX_LEN) == 0) {
    vector<uint8_t> dec_key;
    size_t separator = header_value.find(' ');

    if (separator == string::npos)
      throw runtime_error("header string is malformed.");

    util::decode(header_value.substr(0, separator), E_BASE64, &dec_key);

    ret.reset(val = new xattr_value(reinterpret_cast<char *>(&dec_key[0]), true));

    util::decode(header_value.substr(separator + 1), E_BASE64, &val->_value);

  } else {
    // we know the value doesn't need encoding because it came to us as a valid HTTP string.
    ret.reset(val = new xattr_value(header_key, false, false));

    val->_value.resize(header_value.size());
    memcpy(&val->_value[0], reinterpret_cast<const uint8_t *>(header_value.c_str()), header_value.size());
  }

  return ret;
}

xattr::ptr xattr_value::create(const string &key)
{
  return ptr(new xattr_value(key, !is_key_valid(key)));
}

bool xattr_value::is_read_only()
{
  return false;
}

bool xattr_value::is_serializable()
{
  return true;
}

void xattr_value::set_value(const char *value, size_t size)
{
  _value.resize(size);
  memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

  _encode_value = !is_value_valid(value, size);
}

int xattr_value::get_value(char *buffer, size_t max_size)
{
  size_t size = (_value.size() > max_size) ? max_size : _value.size();

  // so this is a little convoluted, but:
  //
  // if buffer == NULL, return the size of the value
  // if buffer != NULL and value_size > max_size, copy up to max_size and return -ERANGE
  // if buffer != NULL and value_size <= max_size, copy up to value_size and return value_size

  if (buffer == NULL)
    return _value.size();

  memcpy(reinterpret_cast<uint8_t *>(buffer), &_value[0], size);

  return (size == _value.size()) ? size : -ERANGE;
}

void xattr_value::to_header(string *header, string *value)
{
  if (_encode_key || _encode_value) {
    *header = XATTR_HEADER_PREFIX + util::compute_md5(_key, E_HEX);
    *value = util::encode(_key, E_BASE64) + " " + util::encode(&_value[0], _value.size(), E_BASE64);
  } else {
    *header = _key;

    value->resize(_value.size());
    memcpy(&(*value)[0], &_value[0], _value.size());
  }
}

