#include <algorithm>
#include <limits>
#include <list>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "fs/file.h"
#include "fs/local_file_store.h"
#include "fs/object_metadata_cache.h"

using boost::mutex;
using boost::scoped_ptr;
using boost::static_pointer_cast;
using boost::thread;
using std::list;
using std::max;
using std::numeric_limits;
using std::ostream;
using std::string;

using s3::base::config;
using s3::base::statistics;
using s3::fs::file;
using s3::fs::local_file_store;
using s3::fs::object;
using s3::fs::object_metadata_cache;

namespace
{
  typedef list<file::ptr> file_list;

  enum purge_mode
  {
    PURGE_STALE = 0,
    PURGE_ALL   = 1
  };

  const string TEMP_FILE_TEMPLATE = "s3fuse.local-XXXXXX";
  const float PURGE_ADJUSTMENT = 0.9; // aim for 90% of max cache size

  mutex s_mutex;
  scoped_ptr<thread> s_purger;
  size_t s_store_size = 0;
  size_t s_peak_store_size = 0, s_bytes_purged = 0;
  string s_temp_file_template;
  bool s_terminating = false;

  bool increment_until_target_reached(const string &path, const object::ptr &obj, size_t target, file_list *removal_list, size_t *size)
  {
    try {
      file::ptr f;

      if (!obj || obj->get_type() != S_IFREG)
        return true;

      f = static_pointer_cast<file>(obj);

      if (f->is_open())
        return true;

      *size += f->get_local_size();
      removal_list->push_back(f);

      // keep going until we reach our target
      return (*size < target);

    } catch (...) {}

    return true;
  }

  void purge(purge_mode mode)
  {
    mutex::scoped_lock lock(s_mutex);
    size_t starting_size = s_store_size, target_purge_size = 0, purge_size = 0;
    file_list removal_list;

    lock.unlock();

    if (mode == PURGE_STALE) {
      if (starting_size < config::get_max_local_store_size())
        return;

      target_purge_size = starting_size - float(config::get_max_local_store_size()) * PURGE_ADJUSTMENT;
    } else {
      target_purge_size = numeric_limits<size_t>::max();
    }

    object_metadata_cache::for_each_oldest(bind(
      increment_until_target_reached,
      _1,
      _2,
      target_purge_size,
      &removal_list,
      &purge_size));

    for (file_list::const_iterator itor = removal_list.begin(); itor != removal_list.end(); ++itor)
      (*itor)->purge();

    lock.lock();

    S3_LOG(
      LOG_DEBUG,
      "local_file_store::purge",
      "starting size: %zu, target purge size: %zu, purge set size: %zu, ending size: %zu, real purge size: %zu\n",
      starting_size,
      target_purge_size,
      purge_size,
      s_store_size,
      starting_size - s_store_size);

    s_bytes_purged += starting_size - s_store_size;
  }

  void periodic_purge()
  {
    int countdown = config::get_local_store_purge_period();

    while (!s_terminating) {
      if (--countdown == 0) {
        countdown = config::get_local_store_purge_period();
        purge(PURGE_STALE);
      }

      sleep(1);
    }
  }

  void statistics_writer(ostream *o)
  {
    *o <<
      "local file store:\n"
      "  peak size: " << s_peak_store_size << "\n"
      "  bytes purged: " << s_bytes_purged << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

void local_file_store::init()
{
  s_purger.reset(new thread(periodic_purge));

  s_temp_file_template = config::get_local_store_path();

  if (s_temp_file_template.empty() || s_temp_file_template[s_temp_file_template.size() - 1] != '/')
    s_temp_file_template += "/";
 
  s_temp_file_template += TEMP_FILE_TEMPLATE;
}

void local_file_store::terminate()
{
  s_terminating = true;

  s_purger->join();

  purge(PURGE_ALL);

  if (s_store_size)
    S3_LOG(LOG_WARNING, "local_file_store::terminate", "store size is %zu after purging. it should be zero!\n", s_store_size);
}

void local_file_store::increment_store_size(size_t size)
{
  mutex::scoped_lock lock(s_mutex);

  s_store_size += size;
  s_peak_store_size = max(s_peak_store_size, s_store_size);
}

void local_file_store::decrement_store_size(size_t size)
{
  mutex::scoped_lock lock(s_mutex);

  s_store_size -= size;
}

const string & local_file_store::get_temp_file_template()
{
  return s_temp_file_template;
}
