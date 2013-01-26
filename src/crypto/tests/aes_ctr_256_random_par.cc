#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"
#include "crypto/tests/random.h"

using boost::bind;
using boost::thread_group;
using std::vector;

using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;
using s3::crypto::tests::random;

namespace
{
  const int TEST_SIZES[] = { 0, 1, 2, 3, 5, 123, 256, 1023, 1024, 2 * 1024, 64 * 1024 - 1, 1024 * 1024 - 1, 2 * 1024 * 1024, 10 * 1024 * 1024 };
  const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

  const size_t THREADS = 8;
  const size_t CHUNK_SIZE = 8 * 1024;

  void run_thread(bool encrypt, const symmetric_key::ptr &sk, uint8_t *in, uint8_t *out, off_t offset, size_t size)
  {
    while (size) {
      ssize_t sz = (size > CHUNK_SIZE) ? CHUNK_SIZE : size;

      if (encrypt)
        aes_ctr_256::encrypt_with_byte_offset(sk, offset, in + offset, sz, out + offset);
      else
        aes_ctr_256::decrypt_with_byte_offset(sk, offset, in + offset, sz, out + offset);

      offset += sz;
      size -= sz;
    }
  }
}

TEST(aes_ctr_256, random_data_parallel)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    symmetric_key::ptr sk;
    vector<uint8_t> in, out_enc, out_dec;
    bool diff = false;
    size_t size = TEST_SIZES[test];
    size_t num_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    size_t bytes_per_thread = num_chunks / THREADS * CHUNK_SIZE;
    size_t bytes_last_thread = bytes_per_thread + num_chunks % THREADS * CHUNK_SIZE;

    sk = symmetric_key::generate<aes_ctr_256>();

    random::read(size, &in);

    ASSERT_EQ(size, in.size()) << "with size = " << size;

    out_enc.resize(size);
    out_dec.resize(size);

    if (size < THREADS * CHUNK_SIZE) {
      run_thread(true, sk, &in[0], &out_enc[0], 0, size);
    } else {
      thread_group threads;

      for (size_t i = 0; i < THREADS - 1; i++)
        threads.create_thread(bind(&run_thread, true, sk, &in[0], &out_enc[0], i * bytes_per_thread, bytes_per_thread));

      threads.create_thread(bind(&run_thread, true, sk, &in[0], &out_enc[0], (THREADS - 1) * bytes_per_thread, bytes_last_thread));

      threads.join_all();
    }

    if (size < THREADS * CHUNK_SIZE) {
      run_thread(false, sk, &out_enc[0], &out_dec[0], 0, size);
    } else {
      thread_group threads;

      for (size_t i = 0; i < THREADS - 1; i++)
        threads.create_thread(bind(&run_thread, false, sk, &out_enc[0], &out_dec[0], i * bytes_per_thread, bytes_per_thread));

      threads.create_thread(bind(&run_thread, false, sk, &out_enc[0], &out_dec[0], (THREADS - 1) * bytes_per_thread, bytes_last_thread));

      threads.join_all();
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
