#include "s3_async_queue.hh"

using namespace boost;
using namespace std;

using namespace s3;

async_queue::async_queue(int num_threads)
  : _done(false)
{
  for (int i = 0; i < num_threads; i++)
    _threads.create_thread(bind(&async_queue::worker, this));
}

async_queue::~async_queue()
{
  mutex::scoped_lock lock(_mutex);

  _done = false;
  lock.unlock();

  _threads.join_all();
}

void async_queue::worker()
{
  while (true) {
    mutex::scoped_lock lock(_mutex);
    work_function work;

    while (!_done && _queue.empty())
      _condition.wait(lock);

    work = _queue.front();
    _queue.pop_front();
    lock.unlock();

    work();
  }
}

void async_queue::post(const work_function &fn)
{
  mutex::scoped_lock lock(_mutex);

  _queue.push_back(fn);
  _condition.notify_all();
}
