#ifndef S3_FS_MIME_TYPES_H
#define S3_FS_MIME_TYPES_H

#include <string>

namespace s3
{
  namespace fs
  {
    class mime_types
    {
    public:
      static void init();
      static const std::string & get_type_by_extension(std::string ext);
    };
  }
}

#endif
