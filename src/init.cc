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

#include "init.h"
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

using std::runtime_error;
using std::string;

using s3::init;
using s3::base::config;
using s3::base::logger;
using s3::base::request;
using s3::base::statistics;
using s3::base::xml;
using s3::crypto::buffer;
using s3::fs::cache;
using s3::fs::encryption;
using s3::fs::file;
using s3::fs::list_reader;
using s3::fs::mime_types;
using s3::fs::object;
using s3::services::impl;
using s3::services::service;
using s3::threads::pool;

namespace
{
  const int BUCKET_TEST_MAX_RETRIES = 3;
  const int BUCKET_TEST_ID_LEN = 16;

  void test_bucket_access()
  {
    request::ptr req(new request());
    list_reader::ptr reader(new list_reader("/", false, 1));
    xml::element_list list;
    int r = 0;
    int retry_count = 0;

    req->set_hook(service::get_request_hook());
    r = reader->read(req, &list, NULL);
    if (r)
      throw runtime_error("unable to list bocket contents. check bucket name and credentials.");

    while (retry_count++ < BUCKET_TEST_MAX_RETRIES) {
      string rand_url = object::build_internal_url(string("bucket_test_") + buffer::generate(BUCKET_TEST_ID_LEN)->to_string());

      req->init(s3::base::HTTP_HEAD);
      req->set_url(rand_url);
      req->run();

      if (req->get_response_code() != s3::base::HTTP_SC_NOT_FOUND) {
        S3_LOG(LOG_DEBUG, "init::test_bucket_access", "test key exists. that's unusual.\n");
        continue;
      }

      req->init(s3::base::HTTP_PUT);
      req->set_url(rand_url);
      req->set_input_buffer("this is a test.");
      req->run();

      if (req->get_response_code() != s3::base::HTTP_SC_OK) {
        S3_LOG(LOG_WARNING, "init::test_bucket_access", "cannot commit test object to bucket. access to this bucket is likely read-only. continuing anyway, but check permissions if this is unexpected.\n");
      } else {
        req->init(s3::base::HTTP_DELETE);
        req->set_url(rand_url);
        req->run();

        if (req->get_response_code() != s3::base::HTTP_SC_NO_CONTENT)
          S3_LOG(LOG_WARNING, "init::test_bucket_access", "unable to clean up test object. might be missing permission to delete objects. continuing anyway.\n");
      }

      return;
    }

    throw runtime_error("unable to complete bucket access test.");
  }
}

void init::base(int flags, int verbosity, const string &config_file)
{
  logger::init(verbosity);
  config::init(config_file);
  xml::init();

  if ((flags & IB_WITH_STATS) && !config::get_stats_file().empty())
    statistics::init(config::get_stats_file());
}

void init::fs()
{
  file::test_transfer_chunk_sizes();

  cache::init();
  encryption::init();
  mime_types::init();

  test_bucket_access();
}

void init::services()
{
  #ifdef FIXED_SERVICE
    service::init(impl::ptr(new services::FIXED_SERVICE::impl()));
  #else
    #define TEST_SVC(str, ns) \
      do { \
        if (config::get_service() == str) { \
          service::init(impl::ptr(new services::ns::impl())); \
          return; \
        } \
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

    throw runtime_error("invalid service specified.");
  #endif
}

void init::threads()
{
  pool::init();
}

string init::get_enabled_services()
{
  string svcs;

  #ifdef WITH_AWS
    svcs += ", aws";
  #endif

  #ifdef WITH_FVS
    svcs += ", fvs";
  #endif

  #ifdef WITH_GS
    svcs += ", google-storage";
  #endif

  return svcs.empty() ? string("(none)") : svcs.substr(2); // strip leading ", "
}
