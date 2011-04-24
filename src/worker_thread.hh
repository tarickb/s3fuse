#ifndef S3_WORKER_THREAD_HH
#define S3_WORKER_THREAD_HH

#include <pthread.h>

#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

namespace s3
{
  class request;
  class thread_pool;
  class work_item;

  class worker_thread
  {
  public:
    typedef boost::shared_ptr<worker_thread> ptr;

    worker_thread(thread_pool *pool);
    ~worker_thread();

    bool check_timeout(); // return true if thread has hanged

  private:
    void worker();

    thread_pool *_pool;
    boost::thread _thread;
    boost::mutex _mutex;
    boost::shared_ptr<request> _request;
    boost::shared_ptr<work_item> _wi;
    time_t _timeout;
  };
}

#endif
