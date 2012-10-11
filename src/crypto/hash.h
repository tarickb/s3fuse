#ifndef S3_CRYPTO_HASH_H
#define S3_CRYPTO_HASH_H

namespace s3
{
  namespace crypto
  {
    class hash
    {
    public:
      template <class hash_type>
      inline static void compute(const char *input, size_t size, uint8_t *hash)
      {
        hash_type::compute(reinterpret_cast<const uint8_t *>(input), size, hash);
      }

      template <class hash_type>
      inline static void compute(const std::string &input, uint8_t *hash)
      {
        compute<hash_type>(input.c_str(), input.size() + 1, hash);
      }

      template <class hash_type>
      inline static void compute(const std::vector<uint8_t> &input, uint8_t *hash)
      {
        hash_type::compute(&input[0], input.size(), hash);
      }

      template <class hash_type>
      inline static void compute(const std::vector<char> &input, uint8_t *hash)
      {
        compute<hash_type>(&input[0], input.size(), hash);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const uint8_t *input, size_t size)
      {
        uint8_t hash[hash_type::HASH_LEN];

        hash_type::compute(input, size, hash);

        return encoder_type::encode(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::string &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder_type::encode(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const char *input, size_t size)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, size, hash);

        return encoder_type::encode(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::vector<uint8_t> &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder_type::encode(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::vector<char> &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder_type::encode(hash, hash_type::HASH_LEN);
      }
    };
  }
}

#endif
