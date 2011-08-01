#ifndef S3_SYMLINK_HH
#define S3_SYMLINK_HH

#include <string>

#include "object.h"
#include "service.h"
#include "util.h"

namespace s3
{
  class symlink : public object
  {
  public:
    symlink(const std::string &path);

    static const std::string & get_content_type();

    inline static std::string build_url(const std::string &path)
    {
      return service::get_bucket_url() + "/" + util::url_encode(path);
    }

    virtual object_type get_type();
    virtual mode_t get_mode();
  };
}

#endif
