#ifndef S3_FILE_H
#define S3_FILE_H

#include "object.h"

namespace s3
{
  class file : public object
  {
  public:
    file(const std::string &path);
    ~file();

    /*
    inline size_t file::get_size()
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      if (_open_file) {
        struct stat s;

        if (fstat(_open_file->get_fd(), &s) == 0)
          _stat.st_size = s.st_size;
      }

      return _stat.st_size;
    }

    void file::adjust_stat(struct stat *s)
    {
      s->st_size = get_size();
    }
    */
  };
}

#endif
