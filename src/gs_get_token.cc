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

int main(int argc, char **argv) {
  std::string code, access_token, refresh_token;
  time_t expiry;

  if (argc != 2) {
    const char *arg0 = std::strrchr(argv[0], '/');

    std::cerr << "Usage: " << (arg0 ? arg0 + 1 : argv[0]) << "<token-file-name>"
              << std::endl;
    return 1;
  }

  try {
    using s3::services::gs::impl;

    // make sure we can write to the token file before we run the request.
    impl::write_token(argv[1], "");

    std::cout << "Paste this URL into your browser:" << std::endl;
    std::cout << impl::get_new_token_url() << std::endl << std::endl;

    std::cout << "Please enter the authorization code: ";
    getline(std::cin, code);

    impl::get_tokens(impl::GT_AUTH_CODE, code, &access_token, &expiry,
                     &refresh_token);
    impl::write_token(argv[1], refresh_token);

  } catch (const std::exception &e) {
    std::cerr << "Failed to get tokens: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Done!" << std::endl;

  return 0;
}
