#include "logger.h"
#include "request.h"
#include "thread_pool.h"
#include "util.h"

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

  struct _queue_item
  {
    thread_pool::worker_function function;
    async_handle ah;

    inline _queue_item() { }
    inline _queue_item(const thread_pool::worker_function &function_, const async_handle &ah_) : function(function_), ah(ah_) {}

    inline bool is_valid() { return ah; }
  };

  class _worker_thread
  {
  public:
    typedef shared_ptr<_worker_thread> ptr;

    ~_worker_thread()
    {
      if (_time_in_function > 0.0)
        S3_LOG(
          LOG_DEBUG,
          "_worker_thread::~_worker_thread", 
          "time in request/function: %.2f s/%.2f s (%.2f %%)\n", 
          _time_in_request, 
          _time_in_function, 
          (_time_in_request / _time_in_function) * 100.0);
    }

    static ptr create(const thread_pool::ptr &pool)
    {
      ptr wt(new _worker_thread(pool));

      // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
      wt->_thread.reset(new thread(bind(&_worker_thread::worker, wt)));

      return wt;
    }

    bool check_timeout() // return true if thread has hanged
    {
      mutex::scoped_lock lock(_mutex);

      if (_request->check_timeout()) {
        thread_pool::ptr pool = _pool.lock();

        if (pool)
          pool->on_done(_current_ah, -ETIMEDOUT);

        // prevent worker() from continuing
        _pool.reset();
        _current_ah.reset();

        return true;
      }

      return false;
    }

  private:
    _worker_thread(const thread_pool::ptr &pool)
      : _request(new request()),
        _time_in_function(0.),
        _time_in_request(0.),
        _pool(pool)
    { }

    void worker()
    {
      while (true) {
        mutex::scoped_lock lock(_mutex);
        thread_pool::ptr pool;
        _queue_item item;
        int r;

        // the interplay between _mutex and _pool is a little (a lot?) ugly here, but the principles are:
        //
        // 1a. we don't want to hold _mutex while also keeping _pool alive.
        // 1b. we want to minimize the amount of time we keep _pool alive.
        // 2.  we need to lock _mutex when reading/writing _pool or _current_ah (because check_timeout does too).

        pool = _pool.lock();
        lock.unlock();

        if (!pool)
          break;

        item = pool->get_next_queue_item();
        pool.reset();

        if (!item.is_valid())
          break;

        lock.lock();
        _current_ah = item.ah;
        lock.unlock();

        try {
          double start_time, end_time;

          start_time = util::get_current_time();
          _request->reset_current_run_time();

          r = item.function(_request);

          end_time = util::get_current_time();
          _time_in_function += end_time - start_time;
          _time_in_request += _request->get_current_run_time();

        } catch (std::exception &e) {
          S3_LOG(LOG_WARNING, "_worker_thread::worker", "caught exception: %s\n", e.what());
          r = -ECANCELED;

        } catch (...) {
          S3_LOG(LOG_WARNING, "_worker_thread::worker", "caught unknown exception.\n");
          r = -ECANCELED;
        }

        lock.lock();
        pool = _pool.lock();

        if (pool && _current_ah)
          pool->on_done(_current_ah, r);

        _current_ah.reset();
      }

      // the boost::thread in _thread holds a shared_ptr to this, and will keep it from being destructed
      _thread.reset();
    }

    shared_ptr<thread> _thread;
    mutex _mutex;
    request::ptr _request;
    double _time_in_function, _time_in_request;

    // access controlled by _mutex
    weak_ptr<thread_pool> _pool;
    async_handle _current_ah;
  };
}

thread_pool::thread_pool(const string &id)
  : _id(id),
    _done(false),
    _respawn_counter(0)
{
}

thread_pool::ptr thread_pool::create(const string &id, int num_threads)
{
  thread_pool::ptr tp(new thread_pool(id));

  for (int i = 0; i < num_threads; i++)
    tp->_threads.push_back(_worker_thread::create(tp));

  tp->_watchdog_thread.reset(new thread(bind(&thread_pool::watchdog, tp.get())));

  return tp;
}

void thread_pool::terminate()
{
  mutex::scoped_lock lock(_list_mutex);

  _done = true;
  _list_condition.notify_all();
  lock.unlock();

  // shut watchdog down first so it doesn't use _threads
  _watchdog_thread->join();
  _threads.clear();

  // give the threads precisely one second to clean up (and print debug info), otherwise skip them and move on
  usleep(1e6);

  S3_LOG(LOG_DEBUG, "thread_pool::terminate", "[%s] respawn counter: %i.\n", _id.c_str(), _respawn_counter);
}

_queue_item thread_pool::get_next_queue_item()
{
  mutex::scoped_lock lock(_list_mutex);
  _queue_item t;

  while (!_done && _queue.empty())
    _list_condition.wait(lock);

  if (_done)
    return _queue_item(); // generates an invalid queue item

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
  async_handle ah(new _async_handle());
  mutex::scoped_lock lock(_list_mutex);

  _queue.push_back(_queue_item(fn, ah));
  _list_condition.notify_all();

  return ah;
}

int thread_pool::wait(const async_handle &handle)
{
  mutex::scoped_lock lock(_ah_mutex);

  while (!handle->done)
    _ah_condition.wait(lock);

  return handle->return_code;
}

void thread_pool::watchdog()
{
  while (!_done) {
    int respawn = 0;

    for (wt_list::iterator itor = _threads.begin(); itor != _threads.end(); /* do nothing */) {
      if (!(*itor)->check_timeout())
        ++itor;
      else {
        respawn++;
        itor = _threads.erase(itor);
      }
    }

    for (int i = 0; i < respawn; i++)
      _threads.push_back(_worker_thread::create(shared_from_this()));

    _respawn_counter += respawn;
    usleep(1e6);
  }
}
