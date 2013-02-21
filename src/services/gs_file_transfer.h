#include <string>

#include "services/file_transfer.h"
#include "services/multipart_transfer.h"

namespace s3
{
  namespace services
  {
    class gs_file_transfer : public file_transfer
    {
    public:
      gs_file_transfer();

      virtual int upload(const std::string &url, size_t size, const read_chunk_fn &on_read, std::string *returned_etag);

      virtual size_t get_upload_chunk_size();

    private:
      int upload_multi(
        const std::string &url, 
        size_t size, 
        const read_chunk_fn &on_read, 
        std::string *returned_etag);

      size_t _upload_chunk_size;
    };
  }
}
