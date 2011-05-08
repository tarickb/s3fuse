#ifndef S3_FILE_TRANSFER_HH
#define S3_FILE_TRANSFER_HH

#include <string>
#include <boost/shared_ptr.hpp>

namespace s3
{
  class object;
  class request;
  class thread_pool;

  class file_transfer
  {
  public:
    file_transfer(const boost::shared_ptr<thread_pool> &pool);

    int download(const boost::shared_ptr<request> &req, const std::string &url, size_t size, int fd);
    int upload(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj);

  private:
    int download_single(const boost::shared_ptr<request> &req, const std::string &url, int fd, std::string *expected_md5);
    int download_multi(const std::string &url, size_t size, int fd, std::string *expected_md5);

    int upload_single(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj, size_t size);
    int upload_multi(const boost::shared_ptr<request> &req, const boost::shared_ptr<object> &obj, size_t size);

    boost::shared_ptr<thread_pool> _pool;
  };
}

#endif
