#ifndef S3_CRYPTO_HASH_LIST_H
#define S3_CRYPTO_HASH_LIST_H

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    template <class hash_type>
    class hash_list
    {
    public:
      typedef boost::shared_ptr<hash_list<hash_type> > ptr;

      inline hash_list(size_t num_parts)
        : _hashes(num_parts * hash_type::HASH_LEN)
      {
      }

      inline void set_hash_of_part(const size_t part, const uint8_t *data, const size_t size)
      {
        hash_type::compute(data, size, _hashes[part * hash_type::HASH_LEN]);
      }

      inline const std::string & get_root_hash()
      {
        if (_root_hash.empty())
          compute_root_hash();

        return _root_hash;
      }

    private:
      inline void compute_root_hash()
      {
        uint8_t hash[hash_type::HASH_LEN];

        hash_type::compute(&_hashes[0], _hashes.size(), hash);

        _root_hash = util::encode(hash, hash_type::HASH_LEN, E_HEX);
      }

      std::string _root_hash;
      std::vector<uint8_t> _hashes;
    };
  }
}

#endif
