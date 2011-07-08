#include <stdio.h>

#include <iostream>

using namespace std;

const char *CLIENT_ID = "45323348671.apps.googleusercontent.com";
const char *GS_SCOPE = "https://www.googleapis.com/auth/devstorage.read_write";

int main(int argc, char **argv)
{
  string file, code;

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <token-file-name>" << endl;
    return 1;
  }

  file = argv[1];

  cout << "Paste this URL into your browser:" << endl;

  cout << 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" << CLIENT_ID << "&"
    "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
    "scope=" << GS_SCOPE << "&"
    "response_type=code" << endl << endl;

  cout << "Please enter the authorization code: ";
  getline(cin, code);

  cout << "Thank you!" << endl;

  
}
