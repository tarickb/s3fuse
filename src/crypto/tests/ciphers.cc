#include <iostream>

#include "crypto/aes_ctr_128_cipher.h"
#include "crypto/cipher_state.h"

using std::cout;
using std::endl;

using s3::crypto::aes_ctr_128_cipher;
using s3::crypto::cipher_state;

int main(int argc, char **argv)
{
  cipher_state::ptr cs;

  try {
    cs = cipher_state::generate<aes_ctr_128_cipher>();

    cout << "state: " << cs->serialize() << endl;

  } catch (const std::exception &e) {
    cout << "caught exception: " << e.what() << endl;
  }

  return 0;
}
