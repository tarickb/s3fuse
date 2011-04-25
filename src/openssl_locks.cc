#include <pthread.h>
#include <curl/curl.h>
#include <openssl/crypto.h>

#include <stdexcept>
#include <boost/thread.hpp>

#include "logging.hh"
#include "openssl_locks.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  mutex s_mutex;
  int s_ref_count = 0;
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

  void init()
  {
    curl_version_info_data *ver;

    curl_global_init(CURL_GLOBAL_ALL);
    ver = curl_version_info(CURLVERSION_NOW);

    if (!ver)
      throw runtime_error("curl_version_info() failed.");

    S3_DEBUG("openssl_locks::init", "ssl version: %s\n", ver->ssl_version);

    if (strstr(ver->ssl_version, "OpenSSL") == NULL)
      throw runtime_error("curl reports unsupported non-OpenSSL SSL library. cannot continue.");

    s_openssl_locks = static_cast<pthread_mutex_t *>(OPENSSL_malloc(sizeof(*s_openssl_locks) * CRYPTO_num_locks()));

    for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_mutex_init(s_openssl_locks + i, NULL);

    CRYPTO_set_id_callback(openssl_get_thread_id);
    CRYPTO_set_locking_callback(openssl_locking_callback);
  }

  void teardown()
  {
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_mutex_destroy(s_openssl_locks + i);

    OPENSSL_free(s_openssl_locks);
  }
}

void openssl_locks::init()
{
  mutex::scoped_lock lock(s_mutex);

  if (s_ref_count == 0)
    ::init();

  s_ref_count++;
}

void openssl_locks::release()
{
  mutex::scoped_lock lock(s_mutex);

  s_ref_count--;

  if (s_ref_count == 0)
    ::teardown();
}
