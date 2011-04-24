#include "request.hh"
#include "thread_pool.hh"
#include "work_item.hh"
#include "worker_thread.hh"

using namespace boost;
using namespace std;

using namespace s3;

thread_pool::thread_pool(int num_threads)
  : _done(false),
    _timeout_counter(0)
{
  for (int i = 0; i < num_threads; i++)
    _threads.push_back(worker_thread::ptr(new worker_thread(this)));

  _watchdog_thread.reset(new thread(bind(&thread_pool::watchdog, this)));
}

thread_pool::~thread_pool()
{
  {
    mutex::scoped_lock lock(_mutex);

    _done = true;
    _condition.notify_all();
  }

  // shut down watchdog first so it doesn't use _threads
  _watchdog_thread->join();
  _threads.clear();
}

thread_pool::queue_item thread_pool::get_next_queue_item()
{
  mutex::scoped_lock lock(_mutex);
  queue_item t;

  while (!_done && _queue.empty())
    _condition.wait(lock);

  if (_done)
    return queue_item(); // generates an invalid queue item

  t = _queue.front();
  _queue.pop_front();

  return t;
}

void thread_pool::post(const work_item::ptr &wi, int timeout_in_s)
{
  mutex::scoped_lock lock(_mutex);

  _queue.push_back(queue_item(wi, time(NULL) + timeout_in_s));
  _condition.notify_all();
}

void thread_pool::watchdog()
{
  while (!_done) {
    mutex::scoped_lock lock(_mutex);
    int recreate = 0;

    for (wt_list::iterator itor = _threads.begin(); itor != _threads.end(); ++itor)
      if ((*itor)->check_timeout())
        recreate++;

    for (int i = 0; i < recreate; i++)
      _threads.push_back(worker_thread::ptr(new worker_thread(this)));

    lock.unlock();
    sleep(1);
  }
}
