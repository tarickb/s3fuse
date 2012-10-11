#include <errno.h>
#include <string.h>

#include <cctype>
#include <stdexcept>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "objects/value_xattr.h"

using std::string;
using std::runtime_error;
using std::vector;

using namespace s3::crypto;
using namespace s3::objects;

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

xattr::ptr value_xattr::from_header(const string &header_key, const string &header_value)
{
  ptr ret;
  value_xattr *val = NULL;

  if (strncmp(header_key.c_str(), XATTR_HEADER_PREFIX_CSTR, XATTR_HEADER_PREFIX_LEN) == 0) {
    vector<uint8_t> dec_key;
    size_t separator = header_value.find(' ');

    if (separator == string::npos)
      throw runtime_error("header string is malformed.");

    base64::decode(header_value.substr(0, separator), &dec_key);

    ret.reset(val = new value_xattr(reinterpret_cast<char *>(&dec_key[0]), true));

    base64::decode(header_value.substr(separator + 1), &val->_value);

  } else {
    // we know the value doesn't need encoding because it came to us as a valid HTTP string.
    ret.reset(val = new value_xattr(header_key, false, false));

    val->_value.resize(header_value.size());
    memcpy(&val->_value[0], reinterpret_cast<const uint8_t *>(header_value.c_str()), header_value.size());
  }

  return ret;
}

xattr::ptr value_xattr::create(const string &key)
{
  return ptr(new value_xattr(key, !is_key_valid(key)));
}

bool value_xattr::is_read_only()
{
  return false;
}

bool value_xattr::is_serializable()
{
  return true;
}

void value_xattr::set_value(const char *value, size_t size)
{
  _value.resize(size);
  memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

  _encode_value = !is_value_valid(value, size);
}

int value_xattr::get_value(char *buffer, size_t max_size)
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

void value_xattr::to_header(string *header, string *value)
{
  if (_encode_key || _encode_value) {
    *header = XATTR_HEADER_PREFIX + hash::compute<md5, hex>(_key);
    *value = encoder::encode<base64>(_key) + " " + encoder::encode<base64>(_value);
  } else {
    *header = _key;

    value->resize(_value.size());
    memcpy(&(*value)[0], &_value[0], _value.size());
  }
}

