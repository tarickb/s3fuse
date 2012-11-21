#include <map>
#include <boost/thread.hpp>

#include "base/statistics.h"

using boost::mutex;
using std::endl;
using std::make_pair;
using std::multimap;
using std::ostream;
using std::string;

using s3::base::statistics;

namespace
{
  typedef multimap<string, string> stat_map;

  stat_map *s_map = NULL;
  mutex *s_mutex = NULL;
}

void statistics::init()
{
  s_map = new stat_map();
  s_mutex = new mutex();
}

void statistics::write(ostream *o)
{
  if (s_map && s_mutex) {
    mutex::scoped_lock lock(*s_mutex);

    for (stat_map::const_iterator itor = s_map->begin(); itor != s_map->end(); ++itor)
      *o << itor->first << ":" << itor->second << endl;
  }
}

void statistics::post(const string &id, const string &stats)
{
  if (s_map && s_mutex) {
    mutex::scoped_lock lock(*s_mutex);

    s_map->insert(make_pair(id, stats));
  }
}
