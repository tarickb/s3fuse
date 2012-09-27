#ifndef S3_XATTR_VALUE_H
#define S3_XATTR_VALUE_H

#include <stdint.h>

#include "xattr.h"

namespace s3
{
  class xattr_value : public xattr
  {
  public:
    static ptr from_header(const std::string &header_key, const std::string &header_value);
    static ptr create(const std::string &key);

    virtual bool is_serializable();
    virtual bool is_read_only();

    virtual void set_value(const char *value, size_t size);
    virtual int get_value(char *buffer, size_t max_size);

    virtual void to_header(std::string *header, std::string *value);

  private:
    inline xattr_value(const std::string &key, bool encode_key, bool encode_value = true)
      : xattr(key),
        _encode_key(encode_key),
        _encode_value(encode_value)
    {
    }
    
    std::vector<uint8_t> _value;

    bool _encode_key, _encode_value;
  };
}

#endif
