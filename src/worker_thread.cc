#include "logging.hh"

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
    _timeout(0)
{
}

worker_thread::~worker_thread()
{
  if (_pool == NULL)
    return; // don't wait on _thread if it has already timed out

  _pool = NULL;
  _thread.join();
}

bool worker_thread::check_timeout()
{
  mutex::scoped_lock lock(_mutex);

  // TODO: does this work?

  // TODO: allow heartbeats!

  if (_pool && _timeout && time(NULL) > _timeout) {
    _pool = NULL; // prevent worker() from continuing, and prevent subsequent calls here from triggering on_timeout()
    _wi->on_timeout();

    return true;
  }

  return false;
}

void worker_thread::worker()
{
  // TODO: maybe this should hold a shared_ptr to itself? that way this thread won't destruct until this loop completes
  // TODO: on the other hand.. this should only be deleted when the process is shutting down

  while (_pool) {
    thread_pool::queue_item item = _pool->get_next_queue_item();

    if (!item.is_valid())
      return;

    if (item.get_timeout() < time(NULL)) {
      item.get_work_item()->on_timeout();
      continue;
    }

    {
      mutex::scoped_lock lock(_mutex);

      _wi = item.get_work_item();
      _timeout = item.get_timeout();
    }

    item.get_work_item()->exec(_request);

    {
      mutex::scoped_lock lock(_mutex);

      _wi.reset();
      _timeout = 0;
    }
  }

  S3_DEBUG("worker_thread::worker", "exiting.\n");
}

