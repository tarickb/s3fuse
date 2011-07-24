#ifndef S3_TEMP_FILE_H
#define S3_TEMP_FILE_H

namespace s3
{
  class temp_file
  {
  public:
    temp_file();
    ~temp_file();

    inline int get_fd() { return _fd; }

  private:
    int _fd;
  };
}

#endif
