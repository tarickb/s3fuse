#ifndef S3_LOCKED_OBJECT_H
#define S3_LOCKED_OBJECT_H

#include <boost/smart_ptr.hpp>

#include "object.h"

namespace s3
{
  class object_cache;

  class locked_object
  {
  public:
    typedef boost::shared_ptr<locked_object> ptr;

    inline ~locked_object()
    {
      _obj->unlock();
    }

    inline const boost::shared_ptr<object> & get() { return _obj; }

  private:
    friend class object_cache; // for ctor

    inline locked_object(const boost::shared_ptr<object> &obj)
      : _obj(obj)
    {
      _obj->lock();
    }

    boost::shared_ptr<object> _obj;
  };
}

#endif
