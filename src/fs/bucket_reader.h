#include <string>
#include <boost/smart_ptr.hpp>

#include "base/xml.h"

namespace s3
{
  namespace base
  {
    class request;
  };

  namespace fs
  {
    class bucket_reader
    {
    public:
      typedef boost::shared_ptr<bucket_reader> ptr;

      bucket_reader(
        const std::string &prefix, 
        bool group_common_prefixes = true,
        int max_keys = -1);

      int read(
        const boost::shared_ptr<base::request> &req, 
        base::xml::element_list *keys, 
        base::xml::element_list *prefixes);

    private:
      bool _truncated;
      std::string _prefix, _marker;
      bool _group_common_prefixes;
      int _max_keys;
    };
  }
}
