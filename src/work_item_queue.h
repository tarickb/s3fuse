#ifndef S3_WORK_ITEM_QUEUE_H
#define S3_WORK_ITEM_QUEUE_H

#include <deque>

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include "work_item.h"

namespace s3
{
  class work_item_queue
  {
  public:
    typedef boost::shared_ptr<work_item_queue> ptr;

    inline work_item_queue()
      : _done(false)
    {
    }

    inline work_item get_next()
    {
      boost::mutex::scoped_lock lock(_mutex);
      work_item item;

      while (!_done && _queue.empty())
        _condition.wait(lock);

      if (_done)
        return work_item(); // generates an invalid work item

      item = _queue.front();
      _queue.pop_front();

      return item;
    }

    inline void post(const work_item &item)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _queue.push_back(item);
      _condition.notify_all();
    }

    inline void abort()
    {
      boost::mutex::scoped_lock lock(_mutex);

      _done = true;
      _condition.notify_all();
    }

  private:
    boost::mutex _mutex;
    boost::condition _condition;
    std::deque<work_item> _queue;
    bool _done;
  };
}

#endif
