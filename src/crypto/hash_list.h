#ifndef S3_CRYPTO_HASH_LIST_H
#define S3_CRYPTO_HASH_LIST_H

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

#include "crypto/encoder.h"
#include "crypto/hash.h"

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
        hash::compute<hash_type>(data, size, &_hashes[part * hash_type::HASH_LEN]);
      }

      template <class encoder_type>
      inline std::string get_root_hash()
      {
        uint8_t root_hash[hash_type::HASH_LEN];

        hash::compute<hash_type>(_hashes, root_hash);

        return encoder::encode<encoder_type>(root_hash, hash_type::HASH_LEN);
      }

    private:
      std::vector<uint8_t> _hashes;
    };
  }
}

#endif
