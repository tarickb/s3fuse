#include <iostream>

#include "gs_authenticator.h"
#include "logger.h"

using namespace std;

using namespace s3;

int main(int argc, char **argv)
{
  string file, code, access_token, refresh_token;
  time_t expiry;

  logger::init(10);

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <token-file-name>" << endl;
    return 1;
  }

  file = argv[1];

  cout << "Paste this URL into your browser:" << endl;

  cout << 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" << gs_authenticator::get_client_id() << "&"
    "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
    "scope=" << gs_authenticator::get_scope() << "&"
    "response_type=code" << endl << endl;

  cout << "Please enter the authorization code: ";
  getline(cin, code);

  gs_authenticator::get_tokens(code, &access_token, &refresh_token, &expiry);

  cout << "Access token: " << access_token << endl;
  cout << "Refresh token: " << refresh_token << endl;
  cout << "Expiry: " << expiry << endl;

  return 0;
}
