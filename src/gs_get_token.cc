#include <fstream>
#include <iostream>

#include "gs_service_impl.h"
#include "logger.h"

using namespace std;

using namespace s3;

int main(int argc, char **argv)
{
  string file, code, access_token, refresh_token;
  ofstream *out;
  time_t expiry;

  logger::init(10);

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <token-file-name>" << endl;
    return 1;
  }

  out = new ofstream(argv[1], ios_base::out | ios_base::trunc);

  if (!out->good()) {
    cerr << "Unable to open output file [" << argv[1] << "]." << endl;
    return 1;
  }

  cout << "Paste this URL into your browser:" << endl;

  cout << 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" << gs_service_impl::get_client_id() << "&"
    "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
    "scope=" << gs_service_impl::get_oauth_scope() << "&"
    "response_type=code" << endl << endl;

  cout << "Please enter the authorization code: ";
  getline(cin, code);

  try {
    gs_service_impl::get_tokens(gs_service_impl::GT_AUTH_CODE, code, &access_token, &refresh_token, &expiry);

  } catch (const std::exception &e) {
    cerr << "Failed to get tokens: " << e.what() << endl;
    return 1;
  }

  *out << refresh_token << endl;

  cout << "Done!" << endl;

  return 0;
}
