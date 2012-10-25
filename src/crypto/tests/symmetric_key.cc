#include <iostream>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

using std::cout;
using std::endl;

using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;

int main(int argc, char **argv)
{
  symmetric_key::ptr sk;

  sk = symmetric_key::generate<aes_ctr_256>();
  cout << "default sk: " << sk->serialize() << endl;

  sk = symmetric_key::generate<aes_ctr_256>(16);
  cout << "128-bit sk: " << sk->serialize() << endl;

  sk = symmetric_key::generate<aes_ctr_256>(24);
  cout << "192-bit sk: " << sk->serialize() << endl;

  sk = symmetric_key::generate<aes_ctr_256>(32);
  cout << "256-bit sk: " << sk->serialize() << endl;

  return 0;
}
