#ifndef S3_THREADS_REQUEST_WORKER_THREAD_H
#define S3_THREADS_REQUEST_WORKER_THREAD_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class request;

  namespace threads
  {
    class async_handle;
    class work_item_queue;

    class request_worker_thread
    {
    public:
      typedef boost::shared_ptr<request_worker_thread> ptr;

      static ptr create(const boost::shared_ptr<work_item_queue> &queue)
      {
        ptr wt(new request_worker_thread(queue));

        // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
        wt->_thread.reset(new boost::thread(boost::bind(&request_worker_thread::worker, wt)));

        return wt;
      }

      ~request_worker_thread();

      bool check_timeout(); // return true if thread has hanged

    private:
      request_worker_thread(const boost::shared_ptr<work_item_queue> &queue);

      void worker();

      boost::mutex _mutex;
      boost::shared_ptr<boost::thread> _thread;
      boost::shared_ptr<request> _request;
      double _time_in_function, _time_in_request;

      // access controlled by _mutex
      boost::weak_ptr<work_item_queue> _queue;
      boost::shared_ptr<async_handle> _current_ah;
    };
  }
}

#endif
