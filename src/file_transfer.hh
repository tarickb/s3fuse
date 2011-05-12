#ifndef S3_FILE_TRANSFER_HH
#define S3_FILE_TRANSFER_HH

#include <string>
#include <boost/shared_ptr.hpp>

#include "thread_pool.hh"

namespace s3
{
  class object;
  class request;
  class thread_pool;

  class file_transfer
  {
  public:
    file_transfer(const thread_pool::ptr &tp_fg, const thread_pool::ptr &tp_bg);

    inline int download(const std::string &url, size_t size, int fd)
    {
      return _tp_fg->call(boost::bind(&file_transfer::__download, this, _1, url, size, fd));
    }

    inline int upload(const boost::shared_ptr<object> &obj, int fd)
    {
      return _tp_fg->call(boost::bind(&file_transfer::__upload, this, _1, obj, fd));
    }

  private:
    int __download(const boost::shared_ptr<request> &req, const std::string &url, size_t size, int fd);
    int __upload(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj, int fd);

    int download_single(const boost::shared_ptr<request> &req, const std::string &url, int fd, std::string *expected_md5);
    int download_multi(const std::string &url, size_t size, int fd, std::string *expected_md5);

    int upload_single(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj, size_t size, int fd);
    int upload_multi(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj, size_t size, int fd);

    boost::shared_ptr<thread_pool> _tp_fg, _tp_bg;
  };
}

#endif
