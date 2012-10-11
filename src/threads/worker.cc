#include "logger.h"
#include "threads/async_handle.h"
#include "threads/work_item_queue.h"
#include "threads/worker.h"

using boost::shared_ptr;

using s3::threads::worker;

void worker::work()
{
  shared_ptr<request> null_req;

  while (true) {
    work_item item;
    int r;

    item = _queue->get_next();

    if (!item.is_valid())
      break;

    try {
      r = item.get_function()(null_req);

    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "worker::work", "caught exception: %s\n", e.what());
      r = -ECANCELED;

    } catch (...) {
      S3_LOG(LOG_WARNING, "worker::work", "caught unknown exception.\n");
      r = -ECANCELED;
    }

    item.get_ah()->complete(r);
  }

  // the boost::thread in _thread holds a shared_ptr to this, and will keep it from being destructed
  _thread.reset();
}
