/*
 * object_cache.h
 * -------------------------------------------------------------------------
 * Caches object (i.e., file, directory, symlink) metadata.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef S3_OBJECT_CACHE_H
#define S3_OBJECT_CACHE_H

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "logger.h"
#include "locked_object.h"
#include "object.h"
#include "thread_pool.h"

namespace s3
{
  class request;

  class object_cache
  {
  public:
    typedef boost::shared_ptr<object_cache> ptr;

    object_cache(const thread_pool::ptr &pool);
    ~object_cache();

    inline object::ptr get_object(
      const boost::shared_ptr<request> &req, 
      const std::string &path, 
      object_type type_hint = OT_INVALID)
    {
      return get_object(get_lock(req, path, type_hint));
    }

    inline object::ptr get_object(
      const std::string &path, 
      object_type type_hint = OT_INVALID)
    {
      return get_object(get_lock(boost::shared_ptr<request>(), path, type_hint));
    }

    inline locked_object::ptr get_lock(
      const boost::shared_ptr<request> &req, 
      const std::string &path, 
      object_type type_hint = OT_INVALID)
    {
      boost::mutex::scoped_lock lock(_mutex);
      cache_map::iterator itor = _map.find(path);
      object::ptr o;

      if (itor == _map.end() || !itor->second)
        _misses++;
      else {
        o = itor->second;

        if (o->is_valid())
          _hits++;
        else {
          if (o->is_in_use())
            _expired_but_in_use++;
          else {
            _expiries++;
            o.reset();
          }
        }
      }

      if (!o) {
        lock.unlock();

        if (req)
          __fetch(req, path, type_hint, &o);
        else
          _pool->call(boost::bind(&object_cache::__fetch, this, _1, path, type_hint, &o));
        
        if (!o)
          return locked_object::ptr();

        lock.lock();

        itor = _map.find(path);

        if (itor != _map.end() && itor->second && itor->second->is_valid())
          o = itor->second;
        else
          _map[path] = o;
      }

      return locked_object::ptr(new locked_object(o));
    }

    inline void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);
      cache_map::iterator itor = _map.find(path);

      if (itor == _map.end())
        return;

      if (itor->second && !itor->second->is_in_use())
        _map.erase(path);
    }

    /*
    inline open_file::ptr get_file(uint64_t handle)
    {
      boost::mutex::scoped_lock lock(_mutex);
      handle_map::const_iterator itor = _handle_map.find(handle);

      return (itor == _handle_map.end()) ? open_file::ptr() : itor->second->get_open_file();
    }

    int open_handle(const std::string &path, uint64_t *handle)
    {
      boost::mutex::scoped_lock lock(_mutex, boost::defer_lock);
      object::ptr obj = get(path, HINT_IS_FILE);
      open_file::ptr file;

      if (!obj) {
        S3_LOG(LOG_WARNING, "object_cache::open_handle", "cannot open file [%s].\n", path.c_str());
        return -ENOENT;
      }

      lock.lock();
      file = obj->get_open_file();

      if (!file) {
        uint64_t handle = _next_handle++;
        int r;

        file.reset(new open_file(_mutexes, _file_transfer, obj, handle));
        obj->set_open_file(file);

        // handle needs to be in _handle_map before unlocking because a concurrent call to open_handle() for 
        // the same file will block on add_reference(), which expects to have handle in _handle_map on return.
        _handle_map[handle] = obj;

        lock.unlock();
        r = file->init();
        lock.lock();

        if (r) {
          S3_LOG(LOG_WARNING, "object_cache::open_handle", "failed to open file [%s] with error %i.\n", obj->get_path().c_str(), r);

          obj->set_open_file(open_file::ptr());
          _handle_map.erase(handle);
          return r;
        }
      }

      return file->add_reference(handle);
    }

    inline int release_handle(uint64_t handle)
    {
      boost::mutex::scoped_lock lock(_mutex);
      handle_map::iterator itor = _handle_map.find(handle);
      object::ptr obj;

      if (itor == _handle_map.end()) {
        S3_LOG(LOG_WARNING, "object_cache::release_handle", "attempt to release handle not in map.\n");
        return -EINVAL;
      }

      obj = itor->second;

      if (obj->get_open_file()->release()) {
        _handle_map.erase(handle);

        lock.unlock();
        obj->get_open_file()->cleanup();
        lock.lock();

        // keep in _cache_map until cleanup() returns so that any concurrent attempts to open the file fail
        _cache_map.erase(obj->get_path());

        // _obj's reference to the open file is deliberately not reset.  we do this to handle the case where 
        // an object pointer has already been returned by get() but has not yet been used.  an attempt to call
        // open_handle() on this zombie object will fail, but metadata calls will still return more-or-less 
        // correct information (except the MD5 sum, for instance).
      }

      return 0;
    }
    */

  private:
    typedef std::map<std::string, object::ptr> cache_map;

    inline object::ptr get_object(const locked_object::ptr &l)
    {
      return l ? l->get() : object::ptr();
    }

    int __fetch(const request_ptr &req, const std::string &path, object_type type_hint, object::ptr *obj);

    cache_map _map;
    boost::mutex _mutex;
    thread_pool::ptr _pool;
    uint64_t _hits, _misses, _expiries, _expired_but_in_use;
  };
}

#endif
