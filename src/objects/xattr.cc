#include <errno.h>
#include <string.h>

#include <cctype>
#include <stdexcept>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "objects/metadata.h"
#include "objects/xattr.h"

using std::string;
using std::runtime_error;
using std::vector;

using s3::crypto::base64;
using s3::crypto::encoder;
using s3::crypto::hash;
using s3::crypto::hex;
using s3::crypto::md5;
using s3::objects::metadata;
using s3::objects::xattr;

namespace
{
  const size_t  MAX_STRING_SCAN_LEN      = 128;

  inline bool is_key_valid(const string &key)
  {
    if (strncmp(key.c_str(), metadata::RESERVED_PREFIX, strlen(metadata::RESERVED_PREFIX)) == 0)
      return false;

    if (strncmp(key.c_str(), metadata::XATTR_PREFIX, strlen(metadata::XATTR_PREFIX)) == 0)
      return false;

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

xattr::ptr xattr::from_header(const string &header_key, const string &header_value, int mode)
{
  ptr ret;
  xattr *val = NULL;

  if (strncmp(header_key.c_str(), metadata::XATTR_PREFIX, strlen(metadata::XATTR_PREFIX)) == 0) {
    vector<uint8_t> dec_key;
    size_t separator = header_value.find(' ');

    if (separator == string::npos)
      throw runtime_error("header string is malformed.");

    encoder::decode<base64>(header_value.substr(0, separator), &dec_key);

    ret.reset(val = new xattr(reinterpret_cast<char *>(&dec_key[0]), true, true, mode));

    encoder::decode<base64>(header_value.substr(separator + 1), &val->_value);

  } else {
    // we know the value doesn't need encoding because it came to us as a valid HTTP string.
    ret.reset(val = new xattr(header_key, false, false, mode));

    val->_value.resize(header_value.size());
    memcpy(&val->_value[0], reinterpret_cast<const uint8_t *>(header_value.c_str()), header_value.size());
  }

  return ret;
}

xattr::ptr xattr::from_string(const string &key, const string &value, int mode)
{
  ptr ret = create(key, mode);

  // terminating nulls are not stored
  ret->set_value(value.c_str(), value.size());

  return ret;
}

xattr::ptr xattr::create(const string &key, int mode)
{
  return ptr(new xattr(key, !is_key_valid(key), true, mode));
}

void xattr::set_value(const char *value, size_t size)
{
  _value.resize(size);
  memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

  _encode_value = !is_value_valid(value, size);
}

int xattr::get_value(char *buffer, size_t max_size)
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

void xattr::to_header(string *header, string *value)
{
  if (_encode_key || _encode_value) {
    *header = string(metadata::XATTR_PREFIX) + hash::compute<md5, hex>(_key);
    *value = encoder::encode<base64>(_key) + " " + encoder::encode<base64>(_value);
  } else {
    *header = _key;

    value->resize(_value.size());
    memcpy(&(*value)[0], &_value[0], _value.size());
  }
}

