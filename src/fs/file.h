#ifndef S3_FS_FILE_H
#define S3_FS_FILE_H

#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "fs/object.h"
#include "threads/async_handle.h"

namespace s3
{
  namespace fs
  {
    enum file_open_mode
    {
      OPEN_DEFAULT          = 0x0,
      OPEN_TRUNCATE_TO_ZERO = 0x1
    };

    class file : public object
    {
    public:
      typedef boost::shared_ptr<file> ptr;

      inline static file * from_handle(uint64_t handle)
      {
        return reinterpret_cast<file *>(handle);
      }

      static void test_transfer_chunk_sizes();
      static int open(const std::string &path, file_open_mode mode, uint64_t *handle);

      file(const std::string &path);
      virtual ~file();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<file>(object::shared_from_this());
      }

      virtual bool is_removable();

      int release();
      int flush();
      int write(const char *buffer, size_t size, off_t offset);
      int read(char *buffer, size_t size, off_t offset);
      int truncate(off_t length);

      virtual void copy_stat(struct stat *s);

      virtual void set_request_headers(const boost::shared_ptr<base::request> &req);

    protected:
      virtual void init(const boost::shared_ptr<base::request> &req);

      virtual int write_chunk(const char *buffer, size_t size, off_t offset);
      virtual int read_chunk(size_t size, off_t offset, std::vector<char> *buffer);

      virtual int prepare_download();
      virtual int finalize_download();

      virtual int prepare_upload();
      virtual int finalize_upload(const std::string &returned_etag);

      inline const std::string & get_sha256_hash() { return _sha256_hash; }

      void set_sha256_hash(const std::string &hash);

    private:
      struct transfer_part
      {
        int id;
        off_t offset;
        size_t size;
        int retry_count;
        bool success;
        std::string etag;
        threads::wait_async_handle::ptr handle;

        inline transfer_part() : id(0), offset(0), size(0), retry_count(0), success(false) { }
      };

      enum status
      {
        FS_DOWNLOADING = 0x1,
        FS_UPLOADING   = 0x2,
        FS_WRITING     = 0x4,
        FS_DIRTY       = 0x8
      };

      static void open_locked_object(const object::ptr &obj, file_open_mode mode, uint64_t *handle, int *status);

      int open(file_open_mode mode, uint64_t *handle);

      int download(const boost::shared_ptr<base::request> &);

      int download_single(const boost::shared_ptr<base::request> &req);
      int download_multi();
      int download_part(const boost::shared_ptr<base::request> &req, const transfer_part *part);

      int upload(const boost::shared_ptr<base::request> &);

      int upload_single(const boost::shared_ptr<base::request> &req, std::string *returned_etag);
      int upload_multi(std::string *returned_etag);
      int upload_multi_init(const boost::shared_ptr<base::request> &req, std::string *upload_id);
      int upload_multi_cancel(const boost::shared_ptr<base::request> &req, const std::string &upload_id);
      int upload_multi_complete(
        const boost::shared_ptr<base::request> &req, 
        const std::string &upload_id, 
        const std::string &upload_metadata, 
        std::string *etag);
      int upload_part(const boost::shared_ptr<base::request> &req, const std::string &upload_id, transfer_part *part);

      size_t get_transfer_size();

      void on_download_complete(int ret);

      boost::mutex _fs_mutex;
      boost::condition _condition;
      crypto::hash_list<crypto::sha256>::ptr _hash_list;
      std::string _sha256_hash;

      // protected by _fs_mutex
      int _fd, _status, _async_error;
      uint64_t _ref_count;
    };
  }
}

#endif
