#ifndef S3_OPEN_FILE_MAP
#define S3_OPEN_FILE_MAP

namespace s3
{
  class file_transfer;

  class open_file_map
  {
  public:
    inline open_file_map()
      : _next_handle(0)
    {
    }

    inline boost::mutex & get_file_status_mutex() { return _fs_mutex; }
    inline boost::condition & get_file_status_condition() { return _fs_condition; }

    inline const boost::shared_ptr<file_transfer> & get_file_transfer() { return _ft; }

  private:
    typedef boost::shared_ptr<open_file> file_ptr;
    typedef std::map<uint64_t, open_file_ptr> file_map;

    boost::mutex _fs_mutex, _list_mutex;
    boost::condition _fs_condition;
    boost::shared_ptr<file_transfer> _ft;

    file_map _map;
    uint64_t _next_handle;
  };
}

#endif
