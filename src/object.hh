#ifndef S3_OBJECT_HH
#define S3_OBJECT_HH

#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

// TODO: remove
/*

object_from_cache = _object_cache.get(path);
object_from_cache->set_local_file(tmpfile()); // prevents expiry!
_open_file_cache[context] = object::ptr(object_from_cache);

*/

namespace s3
{
  class request;

  enum object_hints
  {
    OH_NONE         = 0x0,
    OH_IS_DIRECTORY = 0x1
  };

  class object
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    inline object(const std::string &path)
      : _path(path),
        _hints(OH_NONE),
        _expiry(0),
        _local_file(NULL)
    { }

    inline void set_hints(int hints) { _hints = hints; }
    inline int get_hints() { return _hints; }

    inline void set_metadata(const std::string &key, const std::string &value) { _metadata[key] = value; }
    inline const std::string & get_metadata(const std::string &key) { return _metadata[key]; }

    inline void set_local_file(FILE *file) { _local_file = file; }
    inline FILE * get_local_file() { return _local_file; }

    inline const std::string & get_content_type() { return _content_type; }
    inline const std::string & get_etag() { return _etag; }
    inline const std::string & get_path() { return _path; }

    inline bool is_valid() { return (_local_file || (_expiry > 0 && _expiry <= time(NULL))); }

    inline const struct stat * get_stats()
    {
      if (_local_file) {
        struct stat s;

        if (fstat(fileno(_local_file), &s) == 0)
          _stat.st_size = s.st_size;
      }

      return &_stat;
    }

  private:
    friend class request; // for request_*

    typedef std::map<std::string, std::string> meta_map;

    void request_init();
    void request_process_header(const std::string &key, const std::string &value);
    void request_process_response(long code, time_t last_modified);
    void request_set_meta_headers(request *req);

    std::string _path, _content_type, _etag;
    int _hints;
    time_t _expiry;
    struct stat _stat;
    FILE *_local_file;
    meta_map _metadata;
  };
}

#endif
