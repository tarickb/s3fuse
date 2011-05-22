#ifndef S3_MUTEXES_HH
#define S3_MUTEXES_HH

#include <boost/thread.hpp>

namespace s3
{
  class mutexes
  {
  public:
    inline boost::mutex & get_object_metadata_mutex() { return _object_metadata; }
    inline boost::mutex & get_object_validity_mutex() { return _object_validity; }
    inline boost::mutex & get_file_status_mutex() { return _file_status; }

  private:
    boost::mutex _object_metadata;
    boost::mutex _object_validity;
    boost::mutex _file_status;
  };
}

#endif
