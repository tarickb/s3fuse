#ifndef S3_DIRECTORY_HH
#define S3_DIRECTORY_HH

#include <list>
#include <boost/function.hpp>

#include "object.h"
#include "service.h"
#include "util.h"

namespace s3
{
  class request;

  class directory : public object
  {
  public:
    typedef boost::function1<void, const std::string &> filler_fn;

    inline static std::string build_url(const std::string &path)
    {
      return service::get_bucket_url() + '/' + util::url_encode(path) + '/';
    }

    directory(const std::string &path);

    virtual object_type get_type();
    virtual mode_t get_mode();

    int fill(const boost::shared_ptr<request> &req, const filler_fn &filler);
    bool is_empty(const boost::shared_ptr<request> &req);

  private:
    typedef std::list<std::string> cache_list;
    typedef boost::shared_ptr<cache_list> cache_list_ptr;

    cache_list_ptr _cache;
  };
}

#endif
