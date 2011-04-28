#ifndef S3_OBJECT_HH
#define S3_OBJECT_HH

#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

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

    // TODO: store bucket, url in member variables?

    static const std::string & get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);

    inline object(const std::string &path)
      : _type(OT_INVALID),
        _path(path),
        _expiry(0),
        _local_file(NULL)
    { }

    void set_defaults(object_type type);

    inline void set_metadata(const std::string &key, const std::string &value) { _metadata[key] = value; }
    inline const std::string & get_metadata(const std::string &key) { return _metadata[key]; }

    inline void set_local_file(FILE *file) { _local_file = file; }
    inline FILE * get_local_file() { return _local_file; }

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    void set_mode(mode_t mode);

    inline object_type get_type() { return _type; }
    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }
    inline const std::string & get_etag() { return _etag; }

    inline bool is_valid() { return (_local_file || (_expiry > 0 && _expiry <= time(NULL))); }

    const std::string & get_url() { return _url; }

    inline void copy_stat(struct stat *s)
    {
      memcpy(s, &_stat, sizeof(_stat));

      if (_local_file) {
        struct stat temp;

        if (fstat(fileno(_local_file), &temp) == 0)
          s->st_size = temp.st_size;
      }
    }

  private:
    friend class request; // for request_*

    typedef std::map<std::string, std::string> meta_map;

    void request_init();
    void request_process_header(const std::string &key, const std::string &value);
    void request_process_response(request *req);
    void request_set_meta_headers(request *req);

    object_type _type;
    std::string _path, _url, _content_type, _etag;
    time_t _expiry;
    struct stat _stat;
    FILE *_local_file;
    meta_map _metadata;
  };
}

#endif
