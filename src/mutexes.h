#ifndef S3_MUTEXES_H
#define S3_MUTEXES_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace s3
{
  class mutexes
  {
  public:
    typedef boost::shared_ptr<mutexes> ptr;

    inline boost::mutex & get_object_metadata_mutex() { return _object_metadata_mutex; }
    inline boost::mutex & get_file_status_mutex() { return _file_status_mutex; }
    inline boost::condition & get_file_status_condition() { return _file_status_condition; }

  private:
    boost::mutex _object_metadata_mutex;
    boost::mutex _file_status_mutex;
    boost::condition _file_status_condition;
  };
}

#endif
