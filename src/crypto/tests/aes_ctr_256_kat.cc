#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/encoder.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"

using std::string;
using std::vector;

using s3::crypto::aes_ctr_256;
using s3::crypto::encoder;
using s3::crypto::hex;
using s3::crypto::symmetric_key;

namespace
{
  struct known_answer
  {
    const char *key;
    const char *iv;
    const char *starting_block;
    const char *plaintext;
    const char *ciphertext;
  };

  const known_answer TESTS[] = {
    // from http://tools.ietf.org/html/rfc3686

    { "ae6852f8121067cc4bf7a5765577f39e", 
      "0000003000000000", 
      "0000000000000001", 
      "53696e676c6520626c6f636b206d7367", 
      "e4095d4fb7a7b3792d6175a3261311b8" },

    { "7e24067817fae0d743d6ce1f32539163", 
      "006cb6dbc0543b59", 
      "da48d90b00000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 
      "5104a106168a72d9790d41ee8edad388eb2e1efc46da57c8fce630df9141be28" },

    { "7691be035e5020a8ac6e618529f9a0dc", 
      "00e0017b27777f3f", 
      "4a1786f000000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223", 
      "c1cf48a89f2ffdd9cf4652e9efdb72d74540a42bde6d7836d59a5ceaaef3105325b2072f" },

    { "16af5b145fc9f579c175f93e3bfb0eed863d06ccfdb78515", 
      "0000004836733c14", 
      "7d6d93cb00000001", 
      "53696e676c6520626c6f636b206d7367", 
      "4b55384fe259c9c84e7935a003cbe928" },

    { "7c5cb2401b3dc33c19e7340819e0f69c678c3db8e6f6a91a", 
      "0096b03b020c6ead", 
      "c2cb500d00000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 
      "453243fc609b23327edfaafa7131cd9f8490701c5ad4a79cfc1fe0ff42f4fb00" },

    { "02bf391ee8ecb159b959617b0965279bf59b60a786d3e0fe", 
      "0007bdfd5cbd6027", 
      "8dcc091200000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223", 
      "96893fc55e5c722f540b7dd1ddf7e758d288bc95c69165884536c811662f2188abee0935" },

    { "776beff2851db06f4c8a0542c8696f6c6a81af1eec96b4d37fc1d689e6c1c104", 
      "00000060db5672c9", 
      "7aa8f0b200000001", 
      "53696e676c6520626c6f636b206d7367", 
      "145ad01dbf824ec7560863dc71e3e0c0" },

    { "f6d66d6bd52d59bb0796365879eff886c66dd51a5b6a99744b50590c87a23884", 
      "00faac24c1585ef1", 
      "5a43d87500000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 
      "f05e231b3894612c49ee000b804eb2a9b8306b508f839d6a5530831d9344af1c" },

    { "ff7a617ce69148e4f1726e2f43581de2aa62d9f805532edff1eed687fb54153d", 
      "001cc5b751a51d70", 
      "a1c1114800000001", 
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223", 
      "eb6c52821d0bbbf7ce7594462aca4faab407df866569fd07f48cc0b583d6071f1ec0e6b8" },

    // from http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf

    { "2b7e151628aed2a6abf7158809cf4f3c", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710", 
      "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee" },

    { "2b7e151628aed2a6abf7158809cf4f3c", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710" },

    { "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710", 
      "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e941e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050" },

    { "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e941e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710" },

    { "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710", 
      "601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c52b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6" },

    { "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", 
      "f0f1f2f3f4f5f6f7", 
      "f8f9fafbfcfdfeff", 
      "601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c52b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6", 
      "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710" }
  };

  const int TEST_COUNT = sizeof(TESTS) / sizeof(TESTS[0]);
}

TEST(aes_ctr_256, known_answers)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    const known_answer *kat = TESTS + test;
    symmetric_key::ptr cs;
    vector<uint8_t> in_buf, out_buf;
    string in_enc, out_enc;
    uint64_t sb = 0;
    string pretty_kat;

    pretty_kat = 
      string("key: ") + kat->key + ", " + 
      "iv: " + kat->iv + ", " + 
      "starting block: " + kat->starting_block + ", " + 
      "plain text: " + kat->plaintext + ", " + 
      "cipher text: " + kat->ciphertext;

    cs = symmetric_key::from_string(string(kat->key) + ":" + kat->iv);

    encoder::decode<hex>(kat->starting_block, &in_buf);
    out_buf.resize(in_buf.size());

    ASSERT_EQ(sizeof(sb), in_buf.size()) << pretty_kat;

    // reverse endianness of starting_block
    for (size_t i = 0; i < in_buf.size(); i++)
      out_buf[in_buf.size() - i - 1] = in_buf[i];

    memcpy(&sb, &out_buf[0], sizeof(sb));

    encoder::decode<hex>(kat->plaintext, &in_buf);
    out_buf.resize(in_buf.size());

    aes_ctr_256::encrypt_with_starting_block(cs, sb, &in_buf[0], in_buf.size(), &out_buf[0]);
    out_enc = encoder::encode<hex>(out_buf);

    EXPECT_EQ(string(kat->ciphertext), out_enc) << pretty_kat;

    aes_ctr_256::decrypt_with_starting_block(cs, sb, &out_buf[0], out_buf.size(), &in_buf[0]);
    in_enc = encoder::encode<hex>(in_buf);

    EXPECT_EQ(string(kat->plaintext), in_enc) << pretty_kat;
  }
}
