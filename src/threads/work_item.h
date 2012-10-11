#ifndef S3_THREADS_WORK_ITEM_H
#define S3_THREADS_WORK_ITEM_H

#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace threads
  {
    class async_handle;

    class work_item
    {
    public:
      typedef boost::function1<int, boost::shared_ptr<base::request> > worker_function;

      inline work_item()
      {
      }

      inline work_item(const worker_function &function, const boost::shared_ptr<async_handle> &ah)
        : _function(function), 
          _ah(ah)
      {
      }

      inline bool is_valid() const { return _ah; }
      inline const boost::shared_ptr<async_handle> & get_ah() const { return _ah; }
      inline const worker_function & get_function() const { return _function; }

    private:
      worker_function _function;
      boost::shared_ptr<async_handle> _ah;
    };
  }
}

#endif
