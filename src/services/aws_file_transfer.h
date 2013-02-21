#include <string>

#include "services/file_transfer.h"
#include "services/multipart_transfer.h"

namespace s3
{
  namespace services
  {
    class aws_file_transfer : public file_transfer
    {
    public:
      aws_file_transfer();

      virtual int upload(const std::string &url, size_t size, const read_chunk_fn &on_read, std::string *returned_etag);

      virtual size_t get_upload_chunk_size();

    private:
      struct upload_range
      {
        int id;
        size_t size;
        off_t offset;
        std::string etag;
      };

      typedef multipart_transfer<upload_range> multipart_upload;

      int upload_part(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id, 
        const read_chunk_fn &on_read, 
        upload_range *range, 
        bool is_retry);

      int upload_multi_init(
        const base::request::ptr &req, 
        const std::string &url, 
        std::string *upload_id);

      int upload_multi_cancel(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id);

      int upload_multi_complete(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id, 
        const std::string &upload_metadata, 
        std::string *etag);

      int upload_multi(
        const std::string &url, 
        size_t size, 
        const read_chunk_fn &on_read, 
        std::string *returned_etag);

      size_t _upload_chunk_size;
    };
  }
}
