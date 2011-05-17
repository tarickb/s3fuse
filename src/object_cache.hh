#ifndef S3_OBJECT_CACHE_HH
#define S3_OBJECT_CACHE_HH

#include <map>
#include <string>
#include <boost/thread.hpp>

#include "object.hh"
#include "thread_pool.hh"

namespace s3
{
  class request;

  enum cache_hints
  {
    HINT_NONE    = 0x0,
    HINT_IS_DIR  = 0x1,
    HINT_IS_FILE = 0x2
  };

  class object_cache
  {
  public:
    object_cache(const thread_pool::ptr &pool);
    ~object_cache();

    inline object::ptr get(const std::string &path, int hints = HINT_NONE)
    {
      object::ptr obj = find(path);

      if (!obj)
        _pool->call(boost::bind(&object_cache::__fetch, this, _1, path, hints, &obj));

      return obj;
    }

    inline object::ptr get(const boost::shared_ptr<request> &req, const std::string &path, int hints = HINT_NONE)
    {
      object::ptr obj = find(path);

      if (!obj)
        __fetch(req, path, hints, &obj);

      // TODO: this should return the object that's already in the map (i.e., relock, if (obj = find(path)) return obj)
      // TODO: so that we don't potentially return several objects
      return obj;
    }

    inline void set(const std::string &path, const object::ptr &object)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _cache[path] = object;
    }

    inline void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _cache.erase(path);
    }

  private:
    typedef std::map<std::string, object::ptr> cache_map;

    inline object::ptr find(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);
      object::ptr &obj = _cache[path];

      if (!obj) {
        _misses++;

      } else if (!obj->is_valid()) {
        _expiries++;
        obj.reset();

      } else {
        _hits++;
      }

      return obj;
    }

    int __fetch(const request_ptr &req, const std::string &path, int hints, object::ptr *obj);

    cache_map _cache;
    boost::mutex _mutex;
    thread_pool::ptr _pool;
    uint64_t _hits, _misses, _expiries;
  };
}

#endif
