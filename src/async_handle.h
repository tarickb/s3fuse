#ifndef S3_ASYNC_HANDLE_H
#define S3_ASYNC_HANDLE_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class async_handle
  {
  public:
    typedef boost::shared_ptr<async_handle> ptr;

    virtual ~async_handle() = 0;

    virtual void complete(int return_code) = 0;
  };

  class wait_async_handle : public async_handle
  {
  public:
    typedef boost::shared_ptr<wait_async_handle> ptr;

    inline wait_async_handle()
      : _return_code(0),
        _done(false)
    {
    }

    inline virtual ~wait_async_handle()
    {
    }

    inline virtual void complete(int return_code)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _return_code = return_code;
      _done = true;

      _condition.notify_all();
    }

    inline int wait()
    {
      boost::mutex::scoped_lock lock(_mutex);

      while (!_done)
        _condition.wait(lock);

      return _return_code;
    }

  private:
    boost::mutex _mutex;
    boost::condition _condition;

    int _return_code;
    bool _done;
  };

  class callback_async_handle : public async_handle
  {
  public:
    typedef boost::shared_ptr<callback_async_handle> ptr;
    typedef boost::function1<void, int> callback_function;

    inline callback_async_handle(const callback_function &cb)
      : _cb(cb)
    {
    }

    inline virtual ~callback_async_handle()
    {
    }

    inline virtual void complete(int return_code)
    {
      _cb(return_code);
    }

  private:
    callback_function _cb;
  };
}

#endif
