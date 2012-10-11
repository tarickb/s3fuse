#ifndef S3_OBJECTS_REFERENCE_XATTR_H
#define S3_OBJECTS_REFERENCE_XATTR_H

#include <boost/thread.hpp>

#include "objects/xattr.h"

namespace s3
{
  namespace objects
  {
    class reference_xattr : public xattr
    {
    public:
      inline static ptr from_string(const std::string &key, std::string *value, boost::mutex *m = NULL)
      {
        xattr::ptr ret;

        ret.reset(new reference_xattr(key, value, m));

        return ret;
      }

      virtual bool is_serializable();
      virtual bool is_read_only();

      virtual void set_value(const char *value, size_t size);
      virtual int get_value(char *buffer, size_t max_size);

      virtual void to_header(std::string *header, std::string *value);
      
    private:
      inline reference_xattr(const std::string &key, std::string *ref, boost::mutex *m)
        : xattr(key),
          _reference(ref),
          _mutex(m)
      {
      }

      std::string *_reference;
      boost::mutex *_mutex;
    };
  }
}

#endif
