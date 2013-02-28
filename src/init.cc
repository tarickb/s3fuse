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
#include "fs/cache.h"
#include "fs/encryption.h"
#include "fs/file.h"
#include "fs/mime_types.h"
#include "services/service.h"
#include "services/aws/impl.h"
#include "services/gs/impl.h"
#include "threads/pool.h"

using std::runtime_error;
using std::string;

using s3::init;
using s3::base::config;
using s3::base::logger;
using s3::base::statistics;
using s3::base::xml;
using s3::fs::cache;
using s3::fs::encryption;
using s3::fs::file;
using s3::fs::mime_types;
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

  cache::init();
  encryption::init();
  mime_types::init();
}

void init::services()
{
  if (config::get_service() == "aws")
    service::init(impl::ptr(new services::aws::impl()));
  else if (config::get_service() == "google-storage")
    service::init(impl::ptr(new services::gs::impl()));
  else
    throw runtime_error("invalid service specified.");
}

void init::threads()
{
  pool::init();
}
