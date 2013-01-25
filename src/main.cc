/*
 * main.cc
 * -------------------------------------------------------------------------
 * FUSE driver for S3.
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

#include <iostream>
#include <string>

#include "operations.h"
#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "base/version.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/encryption.h"
#include "fs/file.h"
#include "services/service.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

using s3::operations;
using s3::base::config;
using s3::base::logger;
using s3::base::statistics;
using s3::base::xml;
using s3::fs::cache;
using s3::fs::encryption;
using s3::fs::file;
using s3::services::service;
using s3::threads::pool;

namespace
{
  struct options
  {
    const char *arg0;
    string config;
    string mountpoint;
    string stats;
    int verbosity;

    #ifdef __APPLE__
      bool noappledouble_set;
      bool daemon_timeout_set;
    #endif
  };
}

int print_usage(const char *arg0)
{
  cerr <<
    "Usage: " << arg0 << " [options] <mountpoint>\n"
    "\n"
    "Options:\n"
    "  -f                   stay in the foreground (i.e., do not daemonize)\n"
    "  -h, --help           print this help message and exit\n"
    "  -o OPT...            pass OPT (comma-separated) to FUSE, such as:\n"
    "     allow_other         allow other users to access the mounted file system\n"
    "     allow_root          allow root to access the mounted file system\n"
    "     config=<file>       use <file> rather than the default configuration file\n"
    "     stats=<file>        write statistics to <file>\n"
    #ifdef __APPLE__
      "     daemon_timeout=<n>  set fuse timeout to <n> seconds\n"
      "     noappledouble       disable testing for/creating .DS_Store files on Mac OS\n"
    #endif
    "  -v, --verbose        enable logging to stderr (can be repeated for more verbosity)\n"
    "  -V, --version        print version and exit\n"
    << endl;

  return 0;
}

int print_version()
{
  cout << s3::base::APP_FULL_NAME << ", " << s3::base::APP_NAME << ", " << s3::base::APP_VERSION << endl;

  return 0;
}

int process_argument(void *data, const char *arg, int key, struct fuse_args *out_args)
{
  options *opts = static_cast<options *>(data);

  if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
    print_version();
    exit(0);
  }

  if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
    print_usage(opts->arg0);
    exit(1);
  }

  if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
    opts->verbosity++;
    return 0;
  }

  if (strstr(arg, "config=") == arg) {
    opts->config = arg + 7;
    return 0;
  }

  if (strstr(arg, "stats=") == arg) {
    opts->stats = arg + 6;
    return 0;
  }

  #ifdef __APPLE__
    if (strstr(arg, "daemon_timeout=") == arg) {
      opts->daemon_timeout_set = true;
      return 1; // continue processing
    }

    if (strstr(arg, "noappledouble") == arg) {
      opts->noappledouble_set = true;
      return 1; // continue processing
    }
  #endif

  if (key == FUSE_OPT_KEY_NONOPT)
    opts->mountpoint = arg; // assume that the mountpoint is the only non-option

  return 1;
}

void build_options(int argc, char **argv, options *opts, fuse_args *args)
{
  opts->verbosity = LOG_WARNING;

  opts->arg0 = strrchr(argv[0], '/');
  opts->arg0 = opts->arg0 ? opts->arg0 + 1 : argv[0];

  #ifdef __APPLE__
    opts->noappledouble_set = false;
    opts->daemon_timeout_set = false;
  #endif

  fuse_opt_parse(args, opts, NULL, process_argument);

  if (opts->mountpoint.empty()) {
    print_usage(opts->arg0);
    exit(1);
  }

  #ifdef __APPLE__
    // TODO: maybe make this and -o noappledouble the default?

    if (!opts->daemon_timeout_set)
      cerr << "Set \"-o daemon_timeout=3600\" or something equally large if transferring large files, otherwise FUSE will time out." << endl;

    if (!opts->noappledouble_set)
      cerr << "You are *strongly* advised to pass \"-o noappledouble\" to disable the creation/checking/etc. of .DS_Store files." << endl;
  #endif
}

#ifdef OSX_BUNDLE
  void set_osx_default_options(int *argc, char ***argv)
  {
    char *arg0 = *argv[0];
    string options, mountpoint, config;

    config = getenv("HOME");
    config += "/.s3fuse/s3fuse.conf";

    try {
      logger::init(LOG_ERR);
      config::init(config);
    } catch (...) {
      exit(1);
    }

    options = "noappledouble";
    options += ",daemon_timeout=3600";
    options += ",config=" + config;
    options += ",volname=s3fuse volume (" + config::get_bucket_name() + ")";

    mountpoint = "/Volumes/s3fuse_" + config::get_bucket_name();

    mkdir(mountpoint.c_str(), 0777);

    *argc = 4;
    *argv = new char *[5];

    (*argv)[0] = arg0;
    (*argv)[1] = strdup("-o");
    (*argv)[2] = strdup(options.c_str());
    (*argv)[3] = strdup(mountpoint.c_str());
    (*argv)[4] = NULL;
  }
#endif

void * init(fuse_conn_info *info)
{
  if (info->capable & FUSE_CAP_ATOMIC_O_TRUNC) {
    info->want |= FUSE_CAP_ATOMIC_O_TRUNC;
    S3_LOG(LOG_DEBUG, "operations::init", "enabling FUSE_CAP_ATOMIC_O_TRUNC\n");
  } else {
    S3_LOG(LOG_WARNING, "operations::init", "FUSE_CAP_ATOMIC_O_TRUNC not supported, will revert to truncate-then-open\n");
  }

  // this has to be here, rather than in main(), because the threads init()
  // creates won't survive the fork in fuse_main().
  pool::init();

  return NULL;
}

int main(int argc, char **argv)
{
  int r;
  options opts;
  fuse_operations opers;

  #ifdef OSX_BUNDLE
    if (argc == 1)
      set_osx_default_options(&argc, &argv);
  #endif

  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  build_options(argc, argv, &opts, &args);

  try {
    logger::init(opts.verbosity);
    config::init(opts.config);

    if (!opts.stats.empty())
      statistics::init(opts.stats);

    service::init(config::get_service());
    xml::init(service::get_xml_namespace());

    cache::init();
    file::test_transfer_chunk_sizes();

    encryption::init();

    operations::init(opts.mountpoint);
    operations::build_fuse_operations(&opers);

    assert(opers.init == NULL);

    // not an actual FS operation
    opers.init = init;

    S3_LOG(LOG_INFO, "main", "%s version %s, initialized\n", s3::base::APP_NAME, s3::base::APP_VERSION);

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while initializing: %s\n", e.what());
    return 1;
  }

  r = fuse_main(args.argc, args.argv, &opers, NULL);

  fuse_opt_free_args(&args);

  try {
    pool::terminate();

    // these won't do anything if statistics::init() wasn't called
    statistics::collect();
    statistics::flush();

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while cleaning up: %s\n", e.what());
  }

  return r;
}
