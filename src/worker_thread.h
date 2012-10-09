#ifndef S3_WORKER_THREAD_H
#define S3_WORKER_THREAD_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class work_item_queue;

  class worker_thread
  {
  public:
    typedef boost::shared_ptr<worker_thread> ptr;

    static ptr create(const boost::shared_ptr<work_item_queue> &queue)
    {
      ptr wt(new worker_thread(queue));

      // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
      wt->_thread.reset(new boost::thread(boost::bind(&worker_thread::worker, wt)));

      return wt;
    }


    inline bool check_timeout() const { return false; }

  private:
    inline worker_thread(const boost::shared_ptr<work_item_queue> &queue)
      : _queue(queue)
    {
    }

    void worker();

    boost::shared_ptr<boost::thread> _thread;
    boost::shared_ptr<work_item_queue> _queue;
  };
}

#endif
