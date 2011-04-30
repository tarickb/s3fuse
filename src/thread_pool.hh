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

#include "work_item.hh"

#define ASYNC_CALL(pool, ns, fn, ...) \
  do { \
    work_item::ptr wi(new work_item(boost::bind(&ns::__ ## fn, this, _1, ## __VA_ARGS__))); \
    \
    (pool)->post(wi); \
    return wi->wait(); \
  } while (0)

#define ASYNC_CALL_NORETURN(pool, ns, fn, ...) \
  do { \
    work_item::ptr wi(new work_item(boost::bind(&ns::__ ## fn, this, _1, ## __VA_ARGS__))); \
    \
    (pool)->post(wi); \
    wi->wait(); \
  } while (0)

#define ASYNC_CALL_NONBLOCK(pool, ns, fn, ...) \
  do { \
    work_item::ptr wi(new work_item(boost::bind(&ns::__ ## fn, this, _1, ## __VA_ARGS__))); \
    \
    (pool)->post(wi); \
  } while (0)

#define ASYNC_CALL_DIRECT(req, ns, fn, ...) \
  do { \
    ns::__ ## fn(req, ## __VA_ARGS__); \
  } while (0)

#define ASYNC_DECL(fn, ...) int __ ## fn (const boost::shared_ptr<request> &req, ## __VA_ARGS__);
#define ASYNC_DEF(ns, fn, ...) int ns::__ ## fn (const boost::shared_ptr<request> &req, ## __VA_ARGS__)

namespace s3
{
  class request;
  class worker_thread;

  class thread_pool
  {
  public:
    typedef boost::shared_ptr<thread_pool> ptr;

    static const int DEFAULT_NUM_THREADS = 8;
    static const int DEFAULT_TIMEOUT_IN_S = 30;

    thread_pool(const std::string &id, int num_threads = DEFAULT_NUM_THREADS);
    ~thread_pool();

    void post(const work_item::ptr &wi, int timeout_in_s = DEFAULT_TIMEOUT_IN_S);

  private:
    friend class worker_thread;

    typedef boost::shared_ptr<worker_thread> wt_ptr;
    typedef std::list<wt_ptr> wt_list;

    class queue_item
    {
    public:
      inline queue_item() : _timeout(0) {}
      inline queue_item(const work_item::ptr &wi, time_t timeout) : _wi(wi), _timeout(timeout) {}

      inline bool is_valid() { return (_timeout > 0); }
      inline const work_item::ptr & get_work_item() { return _wi; }
      inline time_t get_timeout() { return _timeout; }

    private:
      work_item::ptr _wi;
      time_t _timeout;
    };

    queue_item get_next_queue_item();
    void watchdog();

    std::deque<queue_item> _queue;
    wt_list _threads;
    boost::mutex _mutex;
    boost::condition _condition;
    boost::scoped_ptr<boost::thread> _watchdog_thread;
    std::string _id;
    bool _done;
    int _respawn_counter;
  };
}

#endif
