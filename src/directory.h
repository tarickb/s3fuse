#ifndef S3_DIRECTORY_H
#define S3_DIRECTORY_H

#include "object.h"
#include "thread_pool.h"

namespace s3
{
  class directory : public object
  {
  public:
    typedef boost::shared_ptr<directory> ptr;
    typedef boost::function1<void, const std::string &> filler_function;

    static std::string build_url(const std::string &path);
    static void invalidate_parent(const std::string &path);

    directory(const std::string &path);
    virtual ~directory();

    inline ptr shared_from_this()
    {
      return boost::static_pointer_cast<directory>(object::shared_from_this());
    }

    inline int read(const filler_function &filler)
    {
      boost::mutex::scoped_lock lock(_mutex);
      cache_list_ptr cache;

      cache = _cache;
      lock.unlock();

      if (cache) {
        for (cache_list::const_iterator itor = cache->begin(); itor != cache->end(); ++itor)
          filler(*itor);

        return 0;
      } else {
        return thread_pool::call(PR_REQ_0, bind(&directory::read, shared_from_this(), _1, filler));
      }
    }

    bool is_empty(const boost::shared_ptr<request> &req);

    inline bool is_empty()
    {
      return thread_pool::call(PR_REQ_0, boost::bind(&directory::is_empty, shared_from_this(), _1));
    }

    virtual int remove(const boost::shared_ptr<request> &req);
    virtual int rename(const boost::shared_ptr<request> &req, const std::string &to);

  private:
    typedef std::list<std::string> cache_list;
    typedef boost::shared_ptr<cache_list> cache_list_ptr;

    int read(const boost::shared_ptr<request> &req, const filler_function &filler);

    boost::mutex _mutex;
    cache_list_ptr _cache;
  };
}

#endif
