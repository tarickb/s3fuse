#ifndef S3_OBJECT_HH
#define S3_OBJECT_HH

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

#include "open_file.hh"
#include "util.hh"

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

  class object
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    static const std::string & get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);

    inline object(const std::string &path)
      : _type(OT_INVALID),
        _path(path),
        _expiry(0),
        _open_fd(-1)
    { }

    void set_defaults(object_type type);

    inline void set_metadata(const std::string &key, const std::string &value) { _metadata[key] = value; }
    inline const std::string & get_metadata(const std::string &key) { return _metadata[key]; }

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }
    void set_mode(mode_t mode);

    inline object_type get_type() { return _type; }
    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }
    inline const std::string & get_etag() { return _etag; }

    inline bool is_valid() { return (_open_fd != -1 || (_expiry > 0 && time(NULL) < _expiry)); }

    const std::string & get_url() { return _url; }

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

  private:
    friend class request; // for request_*
    friend class open_file_map; // for set_open_file, get_open_file

    typedef std::map<std::string, std::string> meta_map;

    inline void invalidate() { _expiry = 0; _open_fd = -1; } 

    inline const open_file::ptr & get_open_file()
    {
      return _open_file;
    }

    inline void set_open_file(const open_file::ptr &open_file)
    {
      // using _open_file requires a lock (which is held by open_file_map).
      // instead, we'll stick to _open_fd, since operations on it are atomic.

      _open_file = open_file;

      if (open_file)
        _open_fd = open_file->get_fd();
      else
        invalidate();
    }

    void request_init();
    void request_process_header(const std::string &key, const std::string &value);
    void request_process_response(request *req);
    void request_set_meta_headers(request *req);

    object_type _type;
    std::string _path, _url, _content_type, _etag, _mtime_etag;
    time_t _expiry;
    struct stat _stat;
    meta_map _metadata;
    open_file::ptr _open_file;
    int _open_fd;
  };
}

#endif
