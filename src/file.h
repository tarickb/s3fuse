#ifndef S3_FILE_H
#define S3_FILE_H

#include "object.h"
#include "ref_lock.h"

namespace s3
{
  class file : public object
  {
  public:
    typedef ref_lock<file> lock;

    file(const std::string &path);
    ~file();

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
    friend class ref_lock<file>; // for add_ref(), release_ref()

    inline void add_ref()
    {
      boost::mutex::scoped_lock lock(_mutex);

      _ref_count++;
    }

    inline void release_ref()
    {
      boost::mutex::scoped_lock lock(_mutex);

      _ref_count--;
    }

    inline const open_file::ptr & get_open_file()
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      return _open_file;
    }

    inline void set_open_file(const open_file::ptr &open_file)
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      _open_file = open_file;
    }

    boost::mutex _md5_mutex, _open_file_mutex;

    // protected by _md5_mutex
    std::string _md5, _md5_etag;

    // protected by _open_file_mutex
    open_file::ptr _open_file;

    boost::mutex _mutex;
    uint64_t _ref_count;
  };
}

#endif
