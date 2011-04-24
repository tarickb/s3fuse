#ifndef S3_WORK_ITEM_HH
#define S3_WORK_ITEM_HH

#include <stdexcept>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include "logging.hh"

namespace s3
{
  class request;
  class thread_pool;

  class work_item
  {
  public:
    typedef boost::shared_ptr<work_item> ptr;
    typedef boost::function1<int, boost::shared_ptr<request> > worker_function;

    inline work_item(worker_function fn)
      : _fn(fn),
        _return(0),
        _done(false)
    { }

    inline int wait()
    {
      boost::mutex::scoped_lock lock(_mutex);

      while (!_done)
        _condition.wait(lock);

      return _return;
    }

  private:
    friend class worker_thread;

    inline void exec(const boost::shared_ptr<request> &r)
    {
      try {
        _return = _fn(r);

      } catch (std::exception &e) {
        S3_ERROR("work_item::exec", "caught exception: %s\n", e.what());
        _return = -ECANCELED;

      } catch (...) {
        S3_ERROR("work_item::exec", "caught unknown exception.\n");
        _return = -ECANCELED;
      }

      {
        boost::mutex::scoped_lock lock(_mutex);

        _done = true;
        _condition.notify_all();
      }
    }

    inline void on_timeout()
    {
      boost::mutex::scoped_lock lock(_mutex);

      S3_ERROR("work_item::on_timeout", "timed out [%zx].\n", reinterpret_cast<uintptr_t>(this));

      _return = -ETIMEDOUT;
      _done = true;
      _condition.notify_all();
    }

    worker_function _fn;
    int _return;
    bool _done;
    boost::condition _condition;
    boost::mutex _mutex;
  };
}

#endif
