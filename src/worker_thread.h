#ifndef S3_WORKER_THREAD_H
#define S3_WORKER_THREAD_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class async_handle;
  class request;
  class work_item_queue;

  class worker_thread
  {
  public:
    typedef boost::shared_ptr<worker_thread> ptr;

    static ptr create(const boost::shared_ptr<work_item_queue> &queue);

    ~worker_thread();

    bool check_timeout(); // return true if thread has hanged

  private:
    worker_thread(const boost::shared_ptr<work_item_queue> &queue);

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

#endif
