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

#include <pthread.h>
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

#include <stdexcept>
#include <boost/thread.hpp>

#include "curl_easy_handle.h"
#include "logger.h"

using boost::mutex;
using std::runtime_error;

using s3::base::curl_easy_handle;

namespace
{
  mutex s_init_mutex;
  int s_init_count = 0;

  #ifdef HAVE_OPENSSL
    pthread_mutex_t *s_openssl_locks = NULL;

    void openssl_locking_callback(int mode, int n, const char *, int)
    {
      if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(s_openssl_locks + n);
      else
        pthread_mutex_unlock(s_openssl_locks + n);
    }

    unsigned long openssl_get_thread_id()
    {
      return pthread_self();
    }
  #endif

  void pre_init()
  {
    curl_version_info_data *ver;

    curl_global_init(CURL_GLOBAL_ALL);
    ver = curl_version_info(CURLVERSION_NOW);

    if (!ver)
      throw runtime_error("curl_version_info() failed.");

    S3_LOG(LOG_DEBUG, "curl_easy_handle::pre_init", "ssl version: %s\n", ver->ssl_version);

    if (!ver->ssl_version)
      throw runtime_error("curl does not report an SSL library. cannot continue.");

    if (strstr(ver->ssl_version, "NSS"))
      return; // NSS doesn't require external locking

    #ifdef HAVE_GNUTLS
      if (strstr(ver->ssl_version, "GnuTLS")) {
        if (gnutls_global_init() != GNUTLS_E_SUCCESS)
          throw runtime_error("failed to initialize GnuTLS.");

        return;
      }
    #endif

    #ifdef HAVE_OPENSSL
      if (strstr(ver->ssl_version, "OpenSSL")) {
        s_openssl_locks = static_cast<pthread_mutex_t *>(OPENSSL_malloc(sizeof(*s_openssl_locks) * CRYPTO_num_locks()));

        for (int i = 0; i < CRYPTO_num_locks(); i++)
          pthread_mutex_init(s_openssl_locks + i, NULL);

        CRYPTO_set_id_callback(openssl_get_thread_id);
        CRYPTO_set_locking_callback(openssl_locking_callback);

        return;
      }
    #endif

    #ifdef __APPLE__
      if (strstr(ver->ssl_version, "SecureTransport"))
        return;
    #endif

    S3_LOG(LOG_ERR, "curl_easy_handle::pre_init", "unsupported ssl version: %s\n", ver->ssl_version);

    throw runtime_error("curl reports an unsupported ssl library/version.");
  }

  void cleanup()
  {
    #ifdef HAVE_OPENSSL
      if (s_openssl_locks) {
        CRYPTO_set_id_callback(NULL);
        CRYPTO_set_locking_callback(NULL);

        for (int i = 0; i < CRYPTO_num_locks(); i++)
          pthread_mutex_destroy(s_openssl_locks + i);

        OPENSSL_free(s_openssl_locks);
      }
    #endif

    curl_global_cleanup();
  }
}

curl_easy_handle::curl_easy_handle()
{
  mutex::scoped_lock lock(s_init_mutex);

  if (s_init_count++ == 0)
    pre_init();

  _handle = curl_easy_init();

  if (!_handle)
    throw runtime_error("curl_easy_init() failed.");
}

curl_easy_handle::~curl_easy_handle()
{
  mutex::scoped_lock lock(s_init_mutex);

  curl_easy_cleanup(_handle);

  if (--s_init_count == 0)
    cleanup();
}
