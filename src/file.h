#ifndef S3_FILE_HH
#define S3_FILE_HH

#include "object.h"
#include "service.h"
#include "util.h"

namespace s3
{
  class file : public object
  {
  public:
    file(const std::string &path);

    inline static std::string build_url(const std::string &path)
    {
      return service::get_bucket_url() + "/" + util::url_encode(path);
    }

    virtual object_type get_type();
    virtual mode_t get_mode();

    inline void set_md5(const std::string &md5, const std::string &etag)
    {
      boost::mutex::scoped_lock lock(get_mutex());

      _md5 = md5;
      _md5_etag = etag;

      // TODO: set etag?
    }

    inline std::string get_md5()
    {
      boost::mutex::scoped_lock lock(get_mutex());

      return _md5;
    }

    /*
    inline size_t get_size()
    {
      boost::mutex::scoped_lock lock(get_mutex());

      
      if (_open_file)
        _stat.st_size = _open_file->get_size();

      return _stat.st_size;
    }
    */

    virtual void copy_stat(struct stat *s);

    virtual int set_metadata(const std::string &key, const std::string &value, int flags = 0);
    virtual void get_metadata_keys(std::vector<std::string> *keys);
    virtual int get_metadata(const std::string &key, std::string *value);

    virtual void set_meta_headers(const boost::shared_ptr<request> &req);

  protected:
    virtual void build_process_header(const boost::shared_ptr<request> &req, const std::string &key, const std::string &value);
    virtual void build_finalize(const boost::shared_ptr<request> &req);

    virtual bool is_removable();

  private:
    std::string _md5, _md5_etag;
    // open_file::ptr _open_file;
  };
}

#endif
