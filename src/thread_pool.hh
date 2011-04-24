#ifndef S3_THREAD_POOL_HH
#define S3_THREAD_POOL_HH

#include <pthread.h>
#include <sys/time.h>

#include <deque>
#include <list>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class request;
  class worker_thread;
  class work_item;

  class thread_pool
  {
  public:
    typedef boost::shared_ptr<thread_pool> ptr;

    static const int DEFAULT_NUM_THREADS = 8;
    static const int DEFAULT_TIMEOUT_IN_S = 30;

    thread_pool(int num_threads = DEFAULT_NUM_THREADS);
    ~thread_pool();

    void post(const boost::shared_ptr<work_item> &wi, int timeout_in_s = DEFAULT_TIMEOUT_IN_S);

  private:
    friend class worker_thread;

    typedef boost::shared_ptr<work_item> wi_ptr;
    typedef boost::shared_ptr<worker_thread> wt_ptr;
    typedef std::list<wt_ptr> wt_list;

    class queue_item
    {
    public:
      inline queue_item() : _timeout(0) {}
      inline queue_item(const wi_ptr &wi, time_t timeout) : _wi(wi), _timeout(timeout) {}

      inline bool is_valid() { return (_timeout > 0); }
      inline const wi_ptr & get_work_item() { return _wi; }
      inline time_t get_timeout() { return _timeout; }

    private:
      wi_ptr _wi;
      time_t _timeout;
    };

    queue_item get_next_queue_item();
    void watchdog();

    std::deque<queue_item> _queue;
    wt_list _threads;
    boost::mutex _mutex;
    boost::condition _condition;
    boost::scoped_ptr<boost::thread> _watchdog_thread;
    bool _done;
    int _timeout_counter;
  };
}

#endif
