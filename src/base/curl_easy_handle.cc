/*
 * base/curl_easy_handle.cc
 * -------------------------------------------------------------------------
 * Wraps CURL handle, providing init/release tracking, and lock callbacks
 * required to use OpenSSL in a multithreaded application.  See:
 *
 *   http://www.openssl.org/docs/crypto/threads.html
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

#include <string.h>

#if defined(HAVE_OPENSSL) && defined(__APPLE__)
#undef HAVE_OPENSSL // OpenSSL libraries are deprecated on Mac OS
#endif

#ifdef HAVE_OPENSSL
#include <openssl/crypto.h>
#endif

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include <mutex>
#include <stdexcept>

#include "base/curl_easy_handle.h"
#include "base/logger.h"

namespace s3 {
namespace base {

namespace {
std::mutex s_init_mutex;
int s_init_count = 0;

void pre_init() {
  curl_version_info_data *ver;

  curl_global_init(CURL_GLOBAL_ALL);
  ver = curl_version_info(CURLVERSION_NOW);

  if (!ver)
    throw std::runtime_error("curl_version_info() failed.");

  S3_LOG(LOG_DEBUG, "curl_easy_handle::pre_init", "ssl version: %s\n",
         ver->ssl_version);

  if (!ver->ssl_version)
    throw std::runtime_error(
        "curl does not report an SSL library. cannot continue.");

  if (strstr(ver->ssl_version, "NSS"))
    return; // NSS doesn't require external locking

  if (strstr(ver->ssl_version, "OpenSSL"))
    return; // Nothing special required for OpenSSL.

#ifdef HAVE_GNUTLS
  if (strstr(ver->ssl_version, "GnuTLS")) {
    if (gnutls_global_init() != GNUTLS_E_SUCCESS)
      throw std::runtime_error("failed to initialize GnuTLS.");

    return;
  }
#endif

#ifdef __APPLE__
  if (strstr(ver->ssl_version, "SecureTransport"))
    return;
#endif

  S3_LOG(LOG_ERR, "curl_easy_handle::pre_init", "unsupported ssl version: %s\n",
         ver->ssl_version);

  throw std::runtime_error("curl reports an unsupported ssl library/version.");
}

void cleanup() { curl_global_cleanup(); }
} // namespace

curl_easy_handle::curl_easy_handle() {
  std::lock_guard<std::mutex> lock(s_init_mutex);

  if (s_init_count++ == 0)
    pre_init();

  _handle = curl_easy_init();

  if (!_handle)
    throw std::runtime_error("curl_easy_init() failed.");
}

curl_easy_handle::~curl_easy_handle() {
  std::lock_guard<std::mutex> lock(s_init_mutex);

  curl_easy_cleanup(_handle);

  if (--s_init_count == 0)
    cleanup();
}

} // namespace base
} // namespace s3
