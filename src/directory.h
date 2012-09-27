#ifndef S3_DIRECTORY_H
#define S3_DIRECTORY_H

#include "object.h"

namespace s3
{
  class directory : public object
  {
  public:
    typedef std::list<std::string> dir_cache;
    typedef boost::shared_ptr<dir_cache> dir_cache_ptr;

    typedef boost::function1<void, const std::string &> dir_filler_function;

    static std::string build_url(const std::string &path);

    directory(const std::string &path);
    virtual ~directory();

    inline bool is_cached()
    {
      boost::mutex::scoped_lock lock(_dir_mutex);

      return _dir_cache; 
    }

    inline void set_cache(const dir_cache_ptr &dc)
    {
      boost::mutex::scoped_lock lock(_dir_mutex);

      _dir_cache = dc;
    }

    inline void fill(const dir_filler_function &filler)
    {
      boost::mutex::scoped_lock lock(_dir_mutex);
      dir_cache_ptr dc;

      // make local copy of _dir_cache so we don't hold the metadata mutex
      // longer than necessary.

      dc = _dir_cache;
      lock.unlock();

      if (!dc)
        throw std::runtime_error("directory cache not set. cannot fill directory.");

      for (dir_cache::const_iterator itor = dc->begin(); itor != dc->end(); ++itor)
        filler(*itor);
    }

  private:
    boost::mutex _dir_mutex;
    dir_cache_ptr _dir_cache;
  };
}

#endif
