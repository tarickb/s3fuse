#include <iostream>

#include "crypto/aes_ctr_256_cipher.h"
#include "crypto/cipher_state.h"

using std::cout;
using std::endl;

using s3::crypto::aes_ctr_256_cipher;
using s3::crypto::cipher_state;

int main(int argc, char **argv)
{
  cipher_state::ptr cs;

  cs = cipher_state::generate<aes_ctr_256_cipher>();
  cout << "default cs: " << cs->serialize() << endl;

  cs = cipher_state::generate<aes_ctr_256_cipher>(16);
  cout << "128-bit cs: " << cs->serialize() << endl;

  cs = cipher_state::generate<aes_ctr_256_cipher>(24);
  cout << "192-bit cs: " << cs->serialize() << endl;

  cs = cipher_state::generate<aes_ctr_256_cipher>(32);
  cout << "256-bit cs: " << cs->serialize() << endl;

  return 0;
}
