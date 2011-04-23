#include <curl/curl.h>
#include <openssl/crypto.h>

#include <stdexcept>

#include "s3_debug.hh"
#include "s3_request_cache.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
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

  void openssl_init()
  {
    s_openssl_locks = static_cast<pthread_mutex_t *>(OPENSSL_malloc(sizeof(*s_openssl_locks) * CRYPTO_num_locks()));

    for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_mutex_init(s_openssl_locks + i, NULL);

    CRYPTO_set_id_callback(openssl_get_thread_id);
    CRYPTO_set_locking_callback(openssl_locking_callback);
  }

  void openssl_teardown()
  {
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_mutex_destroy(s_openssl_locks + i);

    OPENSSL_free(s_openssl_locks);
  }
}

request_cache::request_cache()
{
  curl_version_info_data *ver;

  curl_global_init(CURL_GLOBAL_ALL);
  ver = curl_version_info(CURLVERSION_NOW);

  if (!ver)
    throw runtime_error("curl_version_info() failed.");

  S3_DEBUG("request_cache::request_cache", "ssl version: %s\n", ver->ssl_version);

  if (strstr(ver->ssl_version, "OpenSSL") == NULL)
    throw runtime_error("curl reports unsupported non-OpenSSL SSL library. cannot continue.");

  openssl_init();
}

request_cache::~request_cache()
{
  mutex::scoped_lock lock(_mutex);

  for (request_vector::iterator itor = _cache.begin(); itor != _cache.end(); ++itor)
    delete *itor;

  openssl_teardown();
}

request_ptr request_cache::get()
{
  mutex::scoped_lock lock(_mutex);
  request *r = NULL;

  for (request_vector::iterator itor = _cache.begin(); itor != _cache.end(); ++itor) {
    if ((*itor)->_ref_count == 0) {
      r = *itor;
      break;
    }
  }

  if (!r) {
    S3_DEBUG("request_cache::get", "no free requests found in cache of size %zu.\n", _cache.size());

    r = new request();
    _cache.push_back(r);
  }

  r->reset();
  return request_ptr(r, true); // call add_ref
}

