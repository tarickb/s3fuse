#include "request.hh"
#include "thread_pool.hh"
#include "work_item.hh"
#include "worker_thread.hh"

using namespace boost;

using namespace s3;

worker_thread::worker_thread(thread_pool *pool)
  : _pool(pool),
    _thread(bind(&worker_thread::worker, this)),
    _request(new request()),
    _timeout(0),
    _pt_id(0)
{
}

worker_thread::~worker_thread()
{
  _thread.join();
}

void worker_thread::check_timeout()
{
  mutex::scoped_lock lock(_mutex);

  // TODO: does this work?

  if (_pool && _timeout && time(NULL) > _timeout) {
    _pool = NULL; // prevent worker() from continuing, and prevent subsequent calls here from triggering on_timeout()
    pthread_cancel(_pt_id);
    _wi->on_timeout();
    _wi.reset();
  }
}

void worker_thread::worker()
{
  _pt_id = pthread_self();

  while (_pool) {
    {
      thread_pool::queue_item item = _pool->get_next_queue_item();

      if (!item.is_valid())
        return;

      if (item.get_timeout() < time(NULL)) {
        item.get_work_item()->on_timeout();
        continue;
      }

      {
        mutex::scoped_lock lock(_mutex);

        // we store the work item shared pointer in _wi rather than here so that we can reset it in cancel() if necessary
        _wi = item.get_work_item();
        _timeout = item.get_timeout();

        // this could fall apart if _wi gets reset between here and exec(), but that's exceedingly unlikely
      }
    }

    _request->reset();
    _wi->exec(_request);
  }
}

