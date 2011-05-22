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

    typedef std::map<std::string, std::string> meta_map;

    static std::string get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);

    inline object(const std::string &path)
      : _type(OT_INVALID),
        _path(path),
        _expiry(0),
        _open_fd(-1)
    { }

    void set_defaults(object_type type);

    int set_metadata(const std::string &key, const std::string &value);

    inline void get_metadata_keys(std::vector<std::string> *keys)
    {
      keys->push_back("__md5__");
      keys->push_back("__etag__");
      keys->push_back("__content_type__");

      for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
        keys->push_back(itor->first);
    }

    inline int get_metadata(const std::string &key, std::string *value)
    {
      meta_map::const_iterator itor;

      if (key == "__md5__")
        *value = _md5;
      else if (key == "__etag__")
        *value = _etag;
      else if (key == "__content_type__")
        *value = _content_type;
      else {
        itor = _metadata.find(key);

        if (itor == _metadata.end())
          return -ENODATA;

        *value = itor->second;
      }

      return 0;
    }

    inline int remove_metadata(const std::string &key)
    {
      meta_map::iterator itor = _metadata.find(key);

      if (itor == _metadata.end())
        return -ENODATA;

      _metadata.erase(itor);
      return 0;
    }

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }
    inline void set_md5(const std::string &md5, const std::string &etag) { _md5 = md5; _md5_etag = etag; _etag = etag; }
    void set_mode(mode_t mode);

    inline object_type get_type() { return _type; }
    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }
    inline const std::string & get_etag() { return _etag; }
    inline const std::string & get_md5() { return _md5; }

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
    friend class object_cache; // for is_valid, set_open_file, get_open_file

    // TODO: merge open_file_map in with this to simplify locking around _open_fd
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

    object_type _type;
    std::string _path, _url, _content_type, _etag, _mtime_etag, _md5, _md5_etag;
    time_t _expiry;
    struct stat _stat;
    meta_map _metadata;
    open_file::ptr _open_file;
    int _open_fd;
  };
}

#endif
