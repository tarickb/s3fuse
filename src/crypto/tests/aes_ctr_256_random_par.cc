#include <algorithm>
#include <functional>
#include <thread>

#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"
#include "crypto/tests/random.h"

namespace s3 {
namespace crypto {
namespace tests {

namespace {
constexpr int TEST_SIZES[] = {0,
                              1,
                              2,
                              3,
                              5,
                              123,
                              256,
                              1023,
                              1024,
                              2 * 1024,
                              64 * 1024 - 1,
                              1024 * 1024 - 1,
                              2 * 1024 * 1024,
                              10 * 1024 * 1024};
constexpr int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

constexpr size_t THREADS = 8;
constexpr size_t CHUNK_SIZE = 8 * 1024;

void RunThread(bool encrypt, const SymmetricKey &sk, uint8_t *in, uint8_t *out,
               off_t offset, size_t size) {
  while (size) {
    ssize_t sz = (size > CHUNK_SIZE) ? CHUNK_SIZE : size;

    if (encrypt)
      AesCtr256::EncryptWithByteOffset(sk, offset, in + offset, sz,
                                       out + offset);
    else
      AesCtr256::DecryptWithByteOffset(sk, offset, in + offset, sz,
                                       out + offset);

    offset += sz;
    size -= sz;
  }
}
}  // namespace

TEST(AesCtr256, RandomDataParallel) {
  for (int test = 0; test < TEST_COUNT; test++) {
    std::vector<uint8_t> in, out_enc, out_dec;
    bool diff = false;
    size_t size = TEST_SIZES[test];
    size_t num_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    size_t bytes_per_thread = num_chunks / THREADS * CHUNK_SIZE;
    size_t bytes_last_thread =
        bytes_per_thread + num_chunks % THREADS * CHUNK_SIZE;

    const auto sk = SymmetricKey::Generate<AesCtr256>();

    in = Random::Read(size);
    ASSERT_EQ(size, in.size()) << "with size = " << size;

    out_enc.resize(size);
    out_dec.resize(size);

    if (size < THREADS * CHUNK_SIZE) {
      RunThread(true, sk, &in[0], &out_enc[0], 0, size);
    } else {
      std::vector<std::thread> threads;

      for (size_t i = 0; i < THREADS - 1; i++)
        threads.emplace_back(std::bind(&RunThread, true, sk, &in[0],
                                       &out_enc[0], i * bytes_per_thread,
                                       bytes_per_thread));

      threads.emplace_back(std::bind(&RunThread, true, sk, &in[0], &out_enc[0],
                                     (THREADS - 1) * bytes_per_thread,
                                     bytes_last_thread));

      std::for_each(threads.begin(), threads.end(),
                    [](std::thread &t) { t.join(); });
    }

    if (size < THREADS * CHUNK_SIZE) {
      RunThread(false, sk, &out_enc[0], &out_dec[0], 0, size);
    } else {
      std::vector<std::thread> threads;

      for (size_t i = 0; i < THREADS - 1; i++)
        threads.emplace_back(std::bind(&RunThread, false, sk, &out_enc[0],
                                       &out_dec[0], i * bytes_per_thread,
                                       bytes_per_thread));

      threads.emplace_back(
          std::bind(&RunThread, false, sk, &out_enc[0], &out_dec[0],
                    (THREADS - 1) * bytes_per_thread, bytes_last_thread));

      std::for_each(threads.begin(), threads.end(),
                    [](std::thread &t) { t.join(); });
    }

    for (size_t i = 0; i < in.size(); i++) {
      if (in[i] != out_dec[i]) {
        diff = true;
        break;
      }
    }

    ASSERT_FALSE(diff) << "with size = " << size;
  }
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
