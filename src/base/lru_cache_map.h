#ifndef S3_BASE_LRU_CACHE_MAP_H
#define S3_BASE_LRU_CACHE_MAP_H

#include <map>
#include <boost/function.hpp>

namespace s3
{
  namespace base
  {
    template <class T>
    inline bool default_removable_test(const T &t) { return true; }

    template
    <
      class key_type, 
      class value_type, 
      bool (*is_removable_fn)(const value_type &v) = default_removable_test<value_type>
    >
    class lru_cache_map
    {
    public:
      typedef boost::function2<void, const key_type &, const value_type &> itor_callback_fn;

      inline lru_cache_map(size_t max_size)
        : _max_size(max_size),
          _newest(NULL),
          _oldest(NULL)
      {
      }

      inline value_type & operator [](const key_type &key)
      {
        entry *e = &_map[key];

        if (e->valid) {
          unlink(e);
        } else {
          e->key = key;
          e->valid = true;

          if (_map.size() > _max_size) {
            entry *to_remove = get_removable();

            if (to_remove)
              erase(to_remove->key);
          }
        }

        make_newest(e);

        return e->value;
      }

      inline void erase(const key_type &key)
      {
        typename map::iterator itor = _map.find(key);
        entry *e = NULL;

        if (itor == _map.end())
          return;

        e = &itor->second;

        unlink(e);
        _map.erase(itor);
      }

      inline void for_each_newest(const itor_callback_fn &cb) const
      {
        entry *e = _newest;

        while (e) {
          cb(e->key, e->value);

          e = e->older;
        }
      }

      inline void for_each_oldest(const itor_callback_fn &cb) const
      {
        entry *e = _oldest;

        while (e) {
          cb(e->key, e->value);

          e = e->newer;
        }
      }

      inline size_t get_size()
      {
        return _map.size();
      }

    private:
      struct entry
      {
        key_type key;
        value_type value;
        entry *older, *newer;
        bool valid;

        entry()
          : older(NULL),
            newer(NULL),
            valid(false)
        {
        }
      };

      typedef std::map<key_type, entry> map;

      inline void unlink(entry *e)
      {
        if (e == _oldest)
          _oldest = e->newer;

        if (e == _newest)
          _newest = e->older;

        if (e->older)
          e->older->newer = e->newer;

        if (e->newer)
          e->newer->older = e->older;

        e->newer = e->older = NULL;
      }

      inline void make_newest(entry *e)
      {
        e->older = _newest;

        if (_newest)
          _newest->newer = e;

        _newest = e;

        if (!_oldest)
          _oldest = e;
      }

      inline entry * get_removable()
      {
        entry *e = _oldest;

        while (e) {
          if (is_removable_fn(e->value))
            break;

          e = e->newer;
        }

        return e;
      }

      map _map;
      size_t _max_size;
      entry *_newest, *_oldest;
    };
  }
}

#endif
