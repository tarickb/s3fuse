#include <iostream>

#include "request.h"

using std::cerr;
using std::cout;
using std::endl;

using s3::request;

void bogus_signer(request *r, bool ignore)
{
  r->set_url("http://www.kernel.org/");
}

int main(int argc, char **argv)
{
  request r;

  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <url>" << endl;
    return 1;
  }

  // r.set_signing_function(bogus_signer);

  r.init(s3::HTTP_GET);
  r.set_url(argv[1]);
  r.run();

  if (r.get_response_code() != s3::HTTP_SC_OK) {
    cerr << "request failed with code: " << r.get_response_code() << endl;
    return 1;
  }

  cout << r.get_output_string() << endl;

  return 0;
}
