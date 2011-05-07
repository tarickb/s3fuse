#ifndef S3_THREAD_POOL_HH
#define S3_THREAD_POOL_HH

#include <pthread.h>
#include <sys/time.h>

#include <deque>
#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class request;

  class _async_handle;
  class _queue_item;
  class _worker_thread;

  typedef boost::shared_ptr<_async_handle> async_handle;

  // for convenience
  typedef boost::shared_ptr<request> request_ptr;

  class thread_pool : public boost::enable_shared_from_this<thread_pool>
  {
  public:
    typedef boost::shared_ptr<thread_pool> ptr;
    typedef boost::function1<int, boost::shared_ptr<request> > worker_function;

    static const int DEFAULT_NUM_THREADS = 8;
    static const int DEFAULT_TIMEOUT_IN_S = 30;

    static ptr create(const std::string &id, int num_threads = DEFAULT_NUM_THREADS);

    inline int call(const worker_function &fn)
    {
      return wait(post(fn));
    }

    inline void call_async(const worker_function &fn)
    {
      post(fn);
    }

    int wait(const async_handle &handle);
    async_handle post(const worker_function &fn, int timeout_in_s = DEFAULT_TIMEOUT_IN_S);
    void terminate();

  private:
    friend class _worker_thread; // for on_done(), get_next_queue_item()

    typedef boost::shared_ptr<_worker_thread> wt_ptr;
    typedef std::list<wt_ptr> wt_list;

    thread_pool(const std::string &id);

    _queue_item get_next_queue_item();
    void on_done(const async_handle &ah, int return_code);
    void watchdog();

    std::deque<_queue_item> _queue;
    wt_list _threads;
    boost::mutex _list_mutex, _ah_mutex;
    boost::condition _list_condition, _ah_condition;
    boost::scoped_ptr<boost::thread> _watchdog_thread;
    std::string _id;
    bool _done;
    int _respawn_counter;
  };
}

#endif
