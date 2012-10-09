#include "async_handle.h"
#include "logger.h"
#include "work_item_queue.h"
#include "worker_thread.h"

using namespace boost;
using namespace std;

using namespace s3;

void worker_thread::worker()
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
      S3_LOG(LOG_WARNING, "worker_thread::worker", "caught exception: %s\n", e.what());
      r = -ECANCELED;

    } catch (...) {
      S3_LOG(LOG_WARNING, "worker_thread::worker", "caught unknown exception.\n");
      r = -ECANCELED;
    }

    item.get_ah()->complete(r);
  }

  // the boost::thread in _thread holds a shared_ptr to this, and will keep it from being destructed
  _thread.reset();
}
