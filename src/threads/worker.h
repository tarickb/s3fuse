#ifndef S3_THREADS_WORKER_H
#define S3_THREADS_WORKER_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  namespace threads
  {
    class work_item_queue;

    class worker
    {
    public:
      typedef boost::shared_ptr<worker> ptr;

      static ptr create(const boost::shared_ptr<work_item_queue> &queue)
      {
        ptr wt(new worker(queue));

        // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
        wt->_thread.reset(new boost::thread(boost::bind(&worker::work, wt)));

        return wt;
      }


      inline bool check_timeout() const { return false; }

    private:
      inline worker(const boost::shared_ptr<work_item_queue> &queue)
        : _queue(queue)
      {
      }

      void work();

      boost::shared_ptr<boost::thread> _thread;
      boost::shared_ptr<work_item_queue> _queue;
    };
  }
}

#endif
