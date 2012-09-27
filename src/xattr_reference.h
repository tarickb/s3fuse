#ifndef S3_XATTR_REFERENCE_H
#define S3_XATTR_REFERENCE_H

#include <boost/thread.hpp>

#include "xattr.h"

namespace s3
{
  class xattr_reference : public xattr
  {
  public:
    inline static ptr from_string(const std::string &key, std::string *value, boost::mutex *m = NULL)
    {
      xattr::ptr ret;

      ret.reset(new xattr_reference(key, value, m));

      return ret;
    }

    virtual bool is_serializable();
    virtual bool is_read_only();

    virtual void set_value(const char *value, size_t size);
    virtual int get_value(char *buffer, size_t max_size);

    virtual void to_header(std::string *header, std::string *value);
    
  private:
    inline xattr_reference(const std::string &key, std::string *ref, boost::mutex *m)
      : xattr(key),
        _reference(ref),
        _mutex(m)
    {
    }

    std::string *_reference;
    boost::mutex *_mutex;
  };
}

#endif
