#include <string>
#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

#include "base/request.h"

namespace s3
{
  namespace services
  {
    class file_transfer
    {
    public:
      typedef boost::function3<int, const char *, size_t, off_t> write_chunk_fn;
      typedef boost::function3<int, size_t, off_t, const base::char_vector_ptr &> read_chunk_fn;

      virtual ~file_transfer();

      virtual size_t get_download_chunk_size();
      virtual size_t get_upload_chunk_size();

      int download(const std::string &url, size_t size, const write_chunk_fn &on_write);
      int upload(const std::string &url, size_t size, const read_chunk_fn &on_read, std::string *returned_etag);

    protected:
      virtual int download_single(
        const base::request::ptr &req, 
        const std::string &url,
        size_t size,
        const write_chunk_fn &on_write);

      virtual int download_multi(
        const std::string &url,
        size_t size,
        const write_chunk_fn &on_write);

      virtual int upload_single(
        const base::request::ptr &req, 
        const std::string &url,
        size_t size,
        const read_chunk_fn &on_read,
        std::string *returned_etag);

      virtual int upload_multi(
        const std::string &url,
        size_t size,
        const read_chunk_fn &on_read,
        std::string *returned_etag);
    };
  }
}
