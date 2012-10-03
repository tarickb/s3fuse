#include "async_handle.h"
#include "logger.h"
#include "request.h"
#include "util.h"
#include "work_item_queue.h"
#include "worker_thread.h"

using namespace boost;
using namespace std;

using namespace s3;

worker_thread::ptr worker_thread::create(const work_item_queue::ptr &queue)
{
  ptr wt(new worker_thread(queue));

  // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
  wt->_thread.reset(new thread(bind(&worker_thread::worker, wt)));

  return wt;
}

worker_thread::worker_thread(const work_item_queue::ptr &queue)
  : _request(new request()),
    _time_in_function(0.),
    _time_in_request(0.),
    _queue(queue)
{
}

worker_thread::~worker_thread()    
{
  if (_time_in_function > 0.0)
    S3_LOG(
      LOG_DEBUG,
      "worker_thread::~worker_thread", 
      "time in request/function: %.2f s/%.2f s (%.2f %%)\n", 
      _time_in_request, 
      _time_in_function, 
      (_time_in_request / _time_in_function) * 100.0);
}

bool worker_thread::check_timeout()
{
  mutex::scoped_lock lock(_mutex);

  if (_request->check_timeout()) {
    _current_ah->complete(-ETIMEDOUT);

    // prevent worker() from continuing
    _queue.reset();
    _current_ah.reset();

    return true;
  }

  return false;
}

void worker_thread::worker()
{
  while (true) {
    mutex::scoped_lock lock(_mutex);
    work_item_queue::ptr queue;
    work_item item;
    int r;

    // the interplay between _mutex and _queue is a little (a lot?) ugly here, but the principles are:
    //
    // 1a. we don't want to hold _mutex while also keeping _queue alive.
    // 1b. we want to minimize the amount of time we keep _queue alive.
    // 2.  we need to lock _mutex when reading/writing _queue or _current_ah (because check_timeout does too).

    queue = _queue.lock();
    lock.unlock();

    if (!queue)
      break;

    item = queue->get_next();
    queue.reset();

    if (!item.is_valid())
      break;

    lock.lock();
    _current_ah = item.get_ah();
    lock.unlock();

    try {
      double start_time, end_time;

      start_time = util::get_current_time();
      _request->reset_current_run_time();

      r = item.get_function()(_request);

      end_time = util::get_current_time();
      _time_in_function += end_time - start_time;
      _time_in_request += _request->get_current_run_time();

    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "worker_thread::worker", "caught exception: %s\n", e.what());
      r = -ECANCELED;

    } catch (...) {
      S3_LOG(LOG_WARNING, "worker_thread::worker", "caught unknown exception.\n");
      r = -ECANCELED;
    }

    lock.lock();

    if (_current_ah) {
      _current_ah->complete(r);

      _current_ah.reset();
    }
  }

  // the boost::thread in _thread holds a shared_ptr to this, and will keep it from being destructed
  _thread.reset();
}

