/*
 * gs_get_token.cc
 * -------------------------------------------------------------------------
 * Gets an OAuth refresh token for Google Storage and saves it in a file.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <iostream>

#include "logger.h"
#include "services/gs_impl.h"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

using s3::services::gs_impl;

int main(int argc, char **argv)
{
  string code, access_token, refresh_token;
  time_t expiry;

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <token-file-name>" << endl;
    return 1;
  }

  try {
    // make sure we can write to the token file before we run the request.
    gs_impl::write_token(argv[1], "");

    cout << "Paste this URL into your browser:" << endl;
    cout << gs_impl::get_new_token_url() << endl << endl;

    cout << "Please enter the authorization code: ";
    getline(cin, code);

    gs_impl::get_tokens(gs_impl::GT_AUTH_CODE, code, &access_token, &expiry, &refresh_token);
    gs_impl::write_token(argv[1], refresh_token);

  } catch (const std::exception &e) {
    cerr << "Failed to get tokens: " << e.what() << endl;
    return 1;
  }

  cout << "Done!" << endl;

  return 0;
}
