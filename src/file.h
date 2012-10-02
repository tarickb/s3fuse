#ifndef S3_FILE_H
#define S3_FILE_H

#include "object.h"
#include "ref_lock.h"

namespace s3
{
  class file : public object
  {
  public:
    typedef boost::shared_ptr<file> ptr;
    typedef ref_lock<file> lock;

    inline static file * from_handle(uint64_t handle)
    {
      return reinterpret_cast<file *>(handle);
    }

    file(const std::string &path);
    ~file();

    virtual bool is_valid();

    inline std::string get_md5()
    {
      boost::mutex::scoped_lock lock(_md5_mutex);

      return _md5;
    }

    inline void set_md5(const std::string &md5, const std::string &etag)
    {
      /*
      boost::mutex::scoped_lock lock(_metadata_mutex);

      _md5 = md5;
      _md5_etag = etag;
      _etag = etag;
      */
    }

    int release();

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

    virtual void set_request_headers(const boost::shared_ptr<request> &req);

  protected:
    virtual void init(const boost::shared_ptr<request> &req);

  private:
    friend class object_cache; // for open()

    enum status
    {
      FS_DOWNLOADING  = 0x1,
      FS_UPLOADING    = 0x2,
      FS_DIRTY        = 0x4
    };

    int open(uint64_t *handle);

    boost::mutex _mutex;
    int _status;
    uint64_t _ref_count;

    // protected by _md5_mutex
    std::string _md5, _md5_etag;

    // protected by _open_file_mutex
    open_file::ptr _open_file;

  };
}

#endif
