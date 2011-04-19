#ifndef S3_ASYNC_QUEUE_HH
#define S3_ASYNC_QUEUE_HH

#include <deque>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class async_queue
  {
  public:
    typedef boost::function0<void> work_function;

    static const int DEFAULT_NUM_THREADS = 4;

    async_queue(int num_threads = DEFAULT_NUM_THREADS);
    ~async_queue();

    void post(const work_function &fn);

  private:
    void worker();

    bool _done;
    std::deque<work_function> _queue;
    boost::mutex _mutex;
    boost::condition _condition;
    boost::thread_group _threads;
  };
}

#endif
