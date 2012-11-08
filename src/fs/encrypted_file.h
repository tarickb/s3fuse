#ifndef S3_FS_ENCRYPTED_FILE_H
#define S3_FS_ENCRYPTED_FILE_H

#include <string>
#include <vector>

#include "fs/file.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace crypto
  {
    class symmetric_key;
  }

  namespace fs
  {
    class encrypted_file : public file
    {
    public:
      typedef boost::shared_ptr<encrypted_file> ptr;

      encrypted_file(const std::string &path);
      virtual ~encrypted_file();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<encrypted_file>(object::shared_from_this());
      }

    protected:
      virtual void init(const boost::shared_ptr<base::request> &req);

      virtual void set_request_headers(const boost::shared_ptr<base::request> &req);

      virtual int prepare_download();
      virtual int finalize_download();

      virtual int prepare_upload();
      virtual int finalize_upload(const std::string &returned_etag);

      virtual int read_chunk(size_t size, off_t offset, std::vector<char> *buffer);
      virtual int write_chunk(const char *buffer, size_t size, off_t offset);

    private:
      boost::shared_ptr<crypto::symmetric_key> _meta_key, _data_key;
      std::string _enc_iv, _enc_meta;
    };
  }
}

#endif
