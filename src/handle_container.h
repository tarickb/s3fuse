#ifndef S3_HANDLE_CONTAINER_H
#define S3_HANDLE_CONTAINER_H

#include <stdint.h>

#include <stdexcept>
#include <boost/smart_ptr.hpp>

#include "file.h"
#include "locked_object.h"

namespace s3
{
  typedef uint64_t object_handle;

  // TODO: merge this with locked_object?
  class handle_container : public boost::noncopyable
  {
  public:
    typedef boost::shared_ptr<handle_container> ptr;

    inline handle_container(const locked_object::ptr &obj, object_handle handle)
      : _object(obj),
        _handle(handle),
        _ref_count(0)
    { }

    inline void add_ref(object_handle *handle)
    {
      ++_ref_count;

      if (handle)
        *handle = _handle;
    }

    inline void release()
    {
      --_ref_count;
    }

    inline bool is_in_use() { return _ref_count > 0; }

    inline file * get_file() { return dynamic_cast<file *>(get_object()); }
    inline object * get_object() { return _object->get().get(); }
    inline const object * get_object() const { return _object->get().get(); }

  private:
    locked_object::ptr _object;
    object_handle _handle;
    boost::detail::atomic_count _ref_count;
  };
}

#endif
