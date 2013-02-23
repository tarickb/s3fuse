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
