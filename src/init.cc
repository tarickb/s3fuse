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
#include "base/statistics.h"
#include "base/xml.h"
#include "fs/encryption.h"
#include "fs/file.h"
#include "fs/local_file.h"
#include "fs/local_file_store.h"
#include "fs/mime_types.h"
#include "fs/object_metadata_cache.h"
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
using s3::base::statistics;
using s3::base::xml;
using s3::fs::encryption;
using s3::fs::file;
using s3::fs::local_file;
using s3::fs::local_file_store;
using s3::fs::mime_types;
using s3::fs::object_metadata_cache;
using s3::services::impl;
using s3::services::service;
using s3::threads::pool;

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

  object_metadata_cache::init();
  encryption::init();
  mime_types::init();
  local_file::init();
  local_file_store::init();
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

void init::cleanup()
{
  local_file_store::terminate();

  pool::terminate();

  // these won't do anything if statistics::init() wasn't called
  statistics::collect();
  statistics::flush();
}
