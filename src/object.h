#ifndef S3_OBJECT_H
#define S3_OBJECT_H

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

#include "mutexes.h"
#include "open_file.h"
#include "util.h"

namespace s3
{
  class request;

  enum object_type
  {
    OT_INVALID,
    OT_FILE,
    OT_DIRECTORY,
    OT_SYMLINK
  };

  class object : public boost::enable_shared_from_this<object>
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    typedef std::map<std::string, std::string> meta_map;

    static std::string get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);

    object(const mutexes::ptr &mutexes, const std::string &path, object_type type = OT_INVALID);

    int set_metadata(const std::string &key, const std::string &value, int flags = 0);
    void get_metadata_keys(std::vector<std::string> *keys);
    int get_metadata(const std::string &key, std::string *value);
    int remove_metadata(const std::string &key);

    void set_mode(mode_t mode);

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }

    inline void set_md5(const std::string &md5, const std::string &etag)
    {
      boost::mutex::scoped_lock lock(_mutexes->get_object_metadata_mutex());

      _md5 = md5;
      _md5_etag = etag;
      _etag = etag;
    }

    inline object_type get_type() { return _type; }
    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }

    inline std::string get_etag()
    {
      boost::mutex::scoped_lock lock(_mutexes->get_object_metadata_mutex());

      return _etag; 
    }

    inline std::string get_md5()
    {
      boost::mutex::scoped_lock lock(_mutexes->get_object_metadata_mutex());

      return _md5; 
    }

    inline const std::string & get_url() { return _url; }

    inline size_t get_size()
    {
      if (_open_fd != -1) {
        struct stat s;

        if (fstat(_open_fd, &s) == 0)
          _stat.st_size = s.st_size;
      }

      return _stat.st_size;
    }

    inline void copy_stat(struct stat *s)
    {
      memcpy(s, &_stat, sizeof(_stat));
      s->st_size = get_size();
    }

    int commit_metadata(const boost::shared_ptr<request> &req);

  private:
    friend class request; // for request_*
    friend class object_cache; // for is_valid, set_open_file, get_open_file

    inline bool is_valid() { return (_expiry > 0 && time(NULL) < _expiry); }

    inline const open_file::ptr & get_open_file()
    {
      return _open_file;
    }

    inline void set_open_file(const open_file::ptr &open_file)
    {
      // this is a one-way call -- we'll never be called with a null open_file
      _open_file = open_file;
      _open_fd = open_file->get_fd();
    }

    void request_init();
    void request_process_header(const std::string &key, const std::string &value);
    void request_process_response(request *req);
    void request_set_meta_headers(request *req);

    mutexes::ptr _mutexes;
    object_type _type;
    std::string _path, _url, _content_type, _mtime_etag;
    time_t _expiry;
    struct stat _stat;

    // protected by _mutexes->get_object_metadata_mutex()
    meta_map _metadata;
    std::string _etag, _md5, _md5_etag;

    // protected by mutex in object_cache
    open_file::ptr _open_file;
    int _open_fd;
  };
}

#endif
