#ifndef S3_OBJECTS_ENCRYPTED_FILE_H
#define S3_OBJECTS_ENCRYPTED_FILE_H

#include <string>
#include <vector>

#include "objects/file.h"

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

  namespace objects
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

      void init(const boost::shared_ptr<base::request> &req);

    protected:
      virtual int prepare_download();
      virtual int finalize_download();

      virtual int prepare_upload();
      virtual int finalize_upload(const std::string &returned_etag);

      virtual int read_chunk(size_t size, off_t offset, std::vector<char> *buffer);
      virtual int write_chunk(const char *buffer, size_t size, off_t offset);

    private:
      boost::shared_ptr<crypto::symmetric_key> _meta_key, _data_key;
      std::string _expected_root_hash;
    };
  }
}

#endif
