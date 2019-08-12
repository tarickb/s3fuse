/*
 * gs_get_token.cc
 * -------------------------------------------------------------------------
 * Gets an OAuth refresh token for Google Storage and saves it in a file.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
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

#include <cstring>
#include <fstream>
#include <iostream>

#include "base/logger.h"
#include "services/gs/impl.h"

const std::string OAUTH_SCOPE =
    "https%3a%2f%2fwww.googleapis.com%2fauth%2fdevstorage.full_control";

int main(int argc, char **argv) {
  if (argc != 4) {
    const char *arg0 = std::strrchr(argv[0], '/');
    std::cerr << "Usage: " << (arg0 ? arg0 + 1 : argv[0])
              << " <client-id> <client-secret> <token-file-name>" << std::endl;
    return 1;
  }

  try {
    using s3::services::gs::GetTokensMode;
    using s3::services::gs::Impl;

    const std::string client_id = argv[1];
    const std::string client_secret = argv[2];
    const std::string file_name = argv[3];

    // make sure we can write to the token file before we run the request.
    Impl::WriteToken(file_name, "");

    const std::string new_token_url =
        "https://accounts.google.com/o/oauth2/auth?"
        "client_id=" +
        client_id +
        "&"
        "redirect_uri=urn%3aietf%3awg%3aoauth%3a2.0%3aoob&"
        "scope=" +
        OAUTH_SCOPE +
        "&"
        "response_type=code";

    std::cout << "Paste this URL into your browser:" << std::endl;
    std::cout << new_token_url << std::endl << std::endl;

    std::cout << "Please enter the authorization code: ";
    std::string code;
    std::getline(std::cin, code);

    auto tokens = Impl::GetTokens(GetTokensMode::AUTH_CODE, client_id,
                                  client_secret, code);
    Impl::WriteToken(file_name, tokens.refresh);
  } catch (const std::exception &e) {
    std::cerr << "Failed to get tokens: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Done!" << std::endl;
  return 0;
}
