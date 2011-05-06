#include "logging.hh"

#include "request.hh"
#include "thread_pool.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace s3
{
  struct _async_handle
  {
    int return_code;
    bool done;

    inline _async_handle() : return_code(0), done(false) { }
  };

  struct queue_item
  {
    thread_pool::worker_function function;
    async_handle ah;
    time_t timeout;

    inline queue_item() : timeout(0) { }
    inline queue_item(const thread_pool::worker_function &function_, const async_handle &ah_, time_t timeout_) : function(function_), ah(ah_), timeout(timeout_) {}

    inline bool is_valid() { return (timeout > 0); }
  };

  class worker_thread
  {
  public:
    typedef shared_ptr<worker_thread> ptr;

    static ptr create(const thread_pool::ptr &pool)
    {
      ptr wt(new worker_thread());

      // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
      wt->_thread.reset(new thread(bind(&worker_thread::worker, wt)));
      wt->_pool = pool;
      wt->_request.reset(new request());
      wt->_timeout = 0;

      return wt;
    }

    void terminate()
    {
      // this is called by thread_pool when it's destructing and wants worker_thread to release its thread_pool pointer
      // it doesn't need to wait for the thread because worker_thread won't destruct until after worker() returns

      mutex::scoped_lock lock(_mutex);

      _pool.reset();
    }

    bool check_timeout() // return true if thread has hanged
    {
      mutex::scoped_lock lock(_mutex);

      // TODO: does this work?
      // TODO: allow heartbeats!

      if (_pool && _timeout && time(NULL) > _timeout) {
        _pool->on_done(_ah, -ETIMEDOUT);
        _pool.reset(); // prevent worker() from continuing, and prevent subsequent calls here from triggering on_timeout()

        return true;
      }

      return false;
    }

  private:
    worker_thread() { }

    void worker()
    {
      while (true) {
        mutex::scoped_lock lock(_mutex);
        queue_item item;
        int r;

        {
          thread_pool::ptr pool;

          // hold a local copy of _pool so that we can call get_next_queue_item(), which can block, without holding
          // _mutex.  this would only ever be a problem if the watchdog in thread_pool calls check_timeout() while
          // holding some lock needed by get_next_queue_item().  unlikely, but possible in the future.

          pool = _pool;
          lock.unlock();

          if (!pool) {
            S3_ERROR("worker_thread::worker", "thread pool pointer no longer valid. exiting.\n");
            return;
          }

          item = pool->get_next_queue_item();
        }

        if (!item.is_valid())
          return;

        lock.lock();

        _ah = item.ah;
        _timeout = item.timeout;

        lock.unlock();

        try {
          r = item.function(_request);

        } catch (std::exception &e) {
          S3_ERROR("worker_thread::worker", "caught exception: %s\n", e.what());
          r = -ECANCELED;

        } catch (...) {
          S3_ERROR("worker_thread::worker", "caught unknown exception.\n");
          r = -ECANCELED;
        }

        lock.lock();

        if (_pool)
          _pool->on_done(_ah, r);

        _ah.reset();
        _timeout = 0;
      }

      S3_DEBUG("worker_thread::worker", "exiting.\n");
    }

    thread_pool::ptr _pool;
    shared_ptr<thread> _thread;
    mutex _mutex;
    shared_ptr<request> _request;
    async_handle _ah;
    time_t _timeout;
  };
}

thread_pool::thread_pool(const string &id, int num_threads)
  : _id(id),
    _done(false),
    _respawn_counter(0)
{
  for (int i = 0; i < num_threads; i++)
    _threads.push_back(worker_thread::create());

  _watchdog_thread.reset(new thread(bind(&thread_pool::watchdog, this)));
}

thread_pool::~thread_pool()
{
  {
    mutex::scoped_lock lock(_list_mutex);

    _done = true;
    _list_condition.notify_all();
  }

  // shut down watchdog first so it doesn't use _threads
  _watchdog_thread->join();
  _threads.clear();

  S3_DEBUG("thread_pool::~thread_pool", "[%s] respawn counter: %i.\n", _id.c_str(), _respawn_counter);
}

queue_item thread_pool::get_next_queue_item()
{
  mutex::scoped_lock lock(_list_mutex);
  queue_item t;

  while (!_done && _queue.empty())
    _list_condition.wait(lock);

  if (_done)
    return queue_item(); // generates an invalid queue item

  // TODO: handle case where _queue.front() has already timed out

  t = _queue.front();
  _queue.pop_front();

  return t;
}

void thread_pool::on_done(const async_handle &ah, int return_code)
{
  mutex::scoped_lock lock(_ah_mutex);

  ah->return_code = return_code;
  ah->done = true;

  _ah_condition.notify_all();
}

async_handle thread_pool::post(const worker_function &fn, int timeout_in_s)
{
  async_handle ah;
  mutex::scoped_lock lock(_list_mutex);

  _queue.push_back(queue_item(fn, ah, time(NULL) + timeout_in_s));
  _list_condition.notify_all();

  return ah;
}

int thread_pool::wait(const async_handle &handle)
{
  boost::mutex::scoped_lock lock(_ah_mutex);

  while (!handle->done)
    _ah_condition.wait(lock);

  return handle->return_code;
}

void thread_pool::watchdog()
{
  while (!_done) {
    int respawn = 0;

    for (wt_list::iterator itor = _threads.begin(); itor != _threads.end(); ++itor)
      if ((*itor)->check_timeout())
        respawn++;

    for (int i = 0; i < respawn; i++)
      _threads.push_back(worker_thread::create(shared_from_this()));

    _respawn_counter += respawn;
    sleep(1);
  }
}
