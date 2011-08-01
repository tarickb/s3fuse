#ifndef S3_DIRECTORY_HH
#define S3_DIRECTORY_HH

namespace s3
{
  class directory : public object
  {
  public:
    typedef boost::function1<void, const std::string &> filler_fn;

    inline static build_url(const std::string &path)
    {
      return service::get_bucket_url() + '/' + util::url_encode(path) + '/';
    }

    directory(const std::string &path);

    virtual object_type get_type();
    virtual mode_t get_mode();

    int fill(const filler_fn &filler);
    bool is_empty();

  private:
    typedef std::list<std::string> cache_list;
    typedef boost::shared_ptr<cache_list> cache_list_ptr;

    cache_list_ptr _cache;
  };
}

#endif
