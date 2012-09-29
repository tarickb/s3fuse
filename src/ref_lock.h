#ifndef S3_REF_LOCK_H
#define S3_REF_LOCK_H

namespace s3
{
  template <class T>
  class ref_lock
  {
  public:
    inline ref_lock()
      : _ptr(NULL)
    {
    }

    inline explicit ref_lock(T *ptr)
      : _ptr(ptr)
    {
      assert(_ptr != NULL);

      _ptr->add_ref();
    }

    inline ref_lock(const ref_lock &l)
      : _ptr(l._ptr)
    {
      assert(_ptr != NULL);

      _ptr->add_ref();
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

    inline T * operator->() { assert(_ptr != NULL); return _ptr; }
    inline T & operator *() { assert(_ptr != NULL); return *_ptr; }

  private:
    T *_ptr;
  };
}

#endif
