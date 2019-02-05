/*
 * init.cc
 * -------------------------------------------------------------------------
 * Initialization helper class implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#include <stdexcept>

#include "base/config.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "crypto/buffer.h"
#include "fs/cache.h"
#include "fs/encryption.h"
#include "fs/file.h"
#include "fs/list_reader.h"
#include "fs/mime_types.h"
#include "fs/object.h"
#include "init.h"
#include "services/service.h"
#include "threads/pool.h"

#ifdef WITH_AWS
#include "services/aws/impl.h"
#endif

#ifdef WITH_FVS
#include "services/fvs/impl.h"
#endif

#ifdef WITH_GS
#include "services/gs/impl.h"
#endif

namespace s3 {

namespace {
const int BUCKET_TEST_MAX_RETRIES = 3;
const int BUCKET_TEST_ID_LEN = 16;

void test_bucket_access() {
  base::request::ptr req(new base::request());
  fs::list_reader::ptr reader(new fs::list_reader("/", false, 1));
  base::xml::element_list list;
  int r = 0;
  int retry_count = 0;

  req->set_hook(services::service::get_request_hook());
  r = reader->read(req, &list, NULL);
  if (r)
    throw std::runtime_error(
        "unable to list bocket contents. check bucket name and credentials.");

  while (retry_count++ < BUCKET_TEST_MAX_RETRIES) {
    std::string rand_url = fs::object::build_internal_url(
        std::string("bucket_test_") +
        crypto::buffer::generate(BUCKET_TEST_ID_LEN)->to_string());

    req->init(s3::base::HTTP_HEAD);
    req->set_url(rand_url);
    req->run();

    if (req->get_response_code() != s3::base::HTTP_SC_NOT_FOUND) {
      S3_LOG(LOG_DEBUG, "init::test_bucket_access",
             "test key exists. that's unusual.\n");
      continue;
    }

    req->init(s3::base::HTTP_PUT);
    req->set_url(rand_url);
    req->set_input_buffer("this is a test.");
    req->run();

    if (req->get_response_code() != s3::base::HTTP_SC_OK) {
      S3_LOG(LOG_WARNING, "init::test_bucket_access",
             "cannot commit test object to bucket. access to this bucket is "
             "likely read-only. continuing anyway, but check permissions if "
             "this is unexpected.\n");
    } else {
      req->init(s3::base::HTTP_DELETE);
      req->set_url(rand_url);
      req->run();

      if (req->get_response_code() != s3::base::HTTP_SC_NO_CONTENT)
        S3_LOG(LOG_WARNING, "init::test_bucket_access",
               "unable to clean up test object. might be missing permission to "
               "delete objects. continuing anyway.\n");
    }

    return;
  }

  throw std::runtime_error("unable to complete bucket access test.");
}
} // namespace

void init::base(int flags, int verbosity, const std::string &config_file) {
  base::logger::init(verbosity);
  base::config::init(config_file);
  base::xml::init();

  if ((flags & IB_WITH_STATS) && !base::config::get_stats_file().empty())
    base::statistics::init(base::config::get_stats_file());
}

void init::fs() {
  fs::file::test_transfer_chunk_sizes();

  fs::cache::init();
  fs::encryption::init();
  fs::mime_types::init();

  test_bucket_access();
}

void init::services() {
#ifdef FIXED_SERVICE
  services::service::init(
      services::impl::ptr(new services::FIXED_SERVICE::impl()));
#else
#define TEST_SVC(str, ns)                                                      \
  do {                                                                         \
    if (base::config::get_service() == str) {                                  \
      services::service::init(services::impl::ptr(new services::ns::impl()));  \
      return;                                                                  \
    }                                                                          \
  } while (0)

#ifdef WITH_AWS
  TEST_SVC("aws", aws);
#endif

#ifdef WITH_FVS
  TEST_SVC("fvs", fvs);
#endif

#ifdef WITH_GS
  TEST_SVC("google-storage", gs);
#endif

#undef TEST_SVC

  throw std::runtime_error("invalid service specified.");
#endif
}

void init::threads() { threads::pool::init(); }

std::string init::get_enabled_services() {
  std::string svcs;

#ifdef WITH_AWS
  svcs += ", aws";
#endif

#ifdef WITH_FVS
  svcs += ", fvs";
#endif

#ifdef WITH_GS
  svcs += ", google-storage";
#endif

  return svcs.empty() ? std::string("(none)")
                      : svcs.substr(2); // strip leading ", "
}

} // namespace s3
