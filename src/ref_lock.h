#ifndef S3_REF_LOCK_H
#define S3_REF_LOCK_H

// TODO: remove!
#include <stdexcept>

#include <assert.h>

#include <boost/smart_ptr.hpp>

namespace s3
{
  template <class T>
  class ref_lock
  {
  public:
    inline ref_lock()
    {
    }

    inline explicit ref_lock(const boost::shared_ptr<T> &ptr)
      : _ptr(ptr)
    {
      assert(_ptr != NULL);

      _ptr->add_ref();
    }

    inline ref_lock(const ref_lock &l)
      : _ptr(l._ptr)
    {
      if (_ptr)
        _ptr->add_ref();
    }

    inline ref_lock & operator =(const ref_lock &l)
    {
      if (_ptr)
        _ptr->release_ref();

      _ptr = l._ptr;
      
      if (_ptr)
        _ptr->add_ref();

      return *this;
    }

    inline ~ref_lock()
    {
      if (_ptr)
        _ptr->release_ref();
    }

    inline bool is_valid()
    {
      return (_ptr != NULL);
    }

    inline T * operator->() { assert(_ptr != NULL); return _ptr.get(); }
    inline T & operator *() { assert(_ptr != NULL); return *_ptr; }

  private:
    boost::shared_ptr<T> _ptr;
  };
}

#endif
