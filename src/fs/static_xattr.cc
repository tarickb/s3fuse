/*
 * fs/static_xattr.cc
 * -------------------------------------------------------------------------
 * Static object extended attribute implementation.
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
#include <string.h>

#include <cctype>
#include <stdexcept>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "fs/metadata.h"
#include "fs/static_xattr.h"

using std::string;
using std::runtime_error;
using std::vector;

using s3::crypto::base64;
using s3::crypto::encoder;
using s3::crypto::hash;
using s3::crypto::hex;
using s3::crypto::md5;
using s3::fs::metadata;
using s3::fs::static_xattr;

namespace
{
  const size_t MAX_STRING_SCAN_LEN = 128;

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

static_xattr::ptr static_xattr::from_header(const string &header_key, const string &header_value, int mode)
{
  ptr ret;
  static_xattr *val = NULL;

  if (strncmp(header_key.c_str(), metadata::XATTR_PREFIX, strlen(metadata::XATTR_PREFIX)) == 0) {
    vector<uint8_t> dec_key;
    size_t separator = header_value.find(' ');

    if (separator == string::npos)
      throw runtime_error("header string is malformed.");

    encoder::decode<base64>(header_value.substr(0, separator), &dec_key);

    ret.reset(val = new static_xattr(reinterpret_cast<char *>(&dec_key[0]), true, true, mode));

    encoder::decode<base64>(header_value.substr(separator + 1), &val->_value);

  } else {
    // we know the value doesn't need encoding because it came to us as a valid HTTP string.
    ret.reset(val = new static_xattr(header_key, false, false, mode));

    val->_value.resize(header_value.size());
    memcpy(&val->_value[0], reinterpret_cast<const uint8_t *>(header_value.c_str()), header_value.size());
  }

  return ret;
}

static_xattr::ptr static_xattr::from_string(const string &key, const string &value, int mode)
{
  ptr ret = create(key, mode);

  // terminating nulls are not stored
  ret->set_value(value.c_str(), value.size());

  return ret;
}

static_xattr::ptr static_xattr::create(const string &key, int mode)
{
  return ptr(new static_xattr(key, !is_key_valid(key), true, mode));
}

int static_xattr::set_value(const char *value, size_t size)
{
  _value.resize(size);
  memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

  _encode_value = !is_value_valid(value, size);

  return 0;
}

int static_xattr::get_value(char *buffer, size_t max_size)
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

void static_xattr::to_header(string *header, string *value)
{
  if (_encode_key || _encode_value) {
    *header = string(metadata::XATTR_PREFIX) + hash::compute<md5, hex>(get_key());
    *value = encoder::encode<base64>(get_key()) + " " + encoder::encode<base64>(_value);
  } else {
    *header = get_key();

    value->resize(_value.size());
    memcpy(&(*value)[0], &_value[0], _value.size());
  }
}
