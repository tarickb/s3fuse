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

#include "init.h"
#include "operations.h"
#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "threads/pool.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

using s3::init;
using s3::operations;
using s3::base::config;
using s3::base::statistics;
using s3::threads::pool;

namespace
{
  const int DEFAULT_VERBOSITY = LOG_WARNING;
  const char *APP_DESCRIPTION = "FUSE driver for cloud object storage services";

  #ifdef __APPLE__
    const string OSX_MOUNTPOINT_PREFIX = "/volumes/" PACKAGE_NAME "_";
  #endif

  struct options
  {
    const char *base_name;
    string config;
    string mountpoint;
    int verbosity;

    #ifdef __APPLE__
      string volname;

      bool noappledouble_set;
      bool daemon_timeout_set;
      bool volname_set;
    #endif

    options(const char *arg0)
    {
      verbosity = DEFAULT_VERBOSITY;

      base_name = strrchr(arg0, '/');
      base_name = base_name ? base_name + 1 : arg0;

      #ifdef __APPLE__
        noappledouble_set = false;
        daemon_timeout_set = false;
        volname_set = false;
      #endif
    }
  };
}

int print_usage(const char *base_name)
{
  cerr <<
    "Usage: " << base_name << " [options] <mountpoint>\n"
    "\n"
    "Options:\n"
    "  -f                   stay in the foreground (i.e., do not daemonize)\n"
    "  -h, --help           print this help message and exit\n"
    "  -o OPT...            pass OPT (comma-separated) to FUSE, such as:\n"
    "     allow_other         allow other users to access the mounted file system\n"
    "     allow_root          allow root to access the mounted file system\n"
    "     default_permissions enforce permissions (useful in multiuser scenarios)\n"
    "     gid=<id>            force group ID for all files to <id>\n"
    "     config=<file>       use <file> rather than the default configuration file\n"
    "     uid=<id>            force user ID for all files to <id>\n"
    "  -v, --verbose        enable logging to stderr (can be repeated for more verbosity)\n"
    "  -vN, --verbose=N     set verbosity to N\n"
    "  -V, --version        print version and exit\n"
    << endl;

  return 0;
}

int print_version()
{
  cout << PACKAGE_NAME << ", " << PACKAGE_VERSION_WITH_REV << ", " << APP_DESCRIPTION << endl;
  cout << "enabled services: " << init::get_enabled_services() << endl;

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
    print_usage(opts->base_name);
    exit(1);
  }

  if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
    opts->verbosity++;
    return 0;
  }

  if (strstr(arg, "-v") == arg) {
    opts->verbosity = atoi(arg + sizeof("-v") - 1); // sizeof counts the trailing null
    return 0;
  }

  if (strstr(arg, "--verbose=") == arg) {
    opts->verbosity = atoi(arg + sizeof("--verbose=") - 1);
    return 0;
  }

  if (strstr(arg, "config=") == arg) {
    opts->config = arg + sizeof("config=") - 1;
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

    if (strstr(arg, "volname=") == arg) {
      opts->volname_set = true;
      return 1; // continue processing
    }
  #endif

  if (key == FUSE_OPT_KEY_NONOPT)
    opts->mountpoint = arg; // assume that the mountpoint is the only non-option

  return 1;
}

void * init(fuse_conn_info *info)
{
  if (info->capable & FUSE_CAP_ATOMIC_O_TRUNC) {
    info->want |= FUSE_CAP_ATOMIC_O_TRUNC;
    S3_LOG(LOG_DEBUG, "operations::init", "enabling FUSE_CAP_ATOMIC_O_TRUNC\n");
  } else {
    S3_LOG(LOG_WARNING, "operations::init", "FUSE_CAP_ATOMIC_O_TRUNC not supported, will revert to truncate-then-open\n");
  }

  // this has to be here, rather than in main(), because the threads created
  // won't survive the fork in fuse_main().
  init::threads();

  return NULL;
}

void add_missing_options(options *opts, fuse_args *args)
{
  #ifdef __APPLE__
    opts->volname = "-ovolname=" PACKAGE_NAME " volume (" + config::get_bucket_name() + ")";

    if (config::get_enable_local_store_persistence())
      fuse_opt_add_arg(args, "-onoubc");

    if (!opts->daemon_timeout_set)
      fuse_opt_add_arg(args, "-odaemon_timeout=3600");

    if (!opts->noappledouble_set)
      fuse_opt_add_arg(args, "-onoappledouble");

    if (!opts->volname_set)
      fuse_opt_add_arg(args, opts->volname.c_str());
  #endif
}

int main(int argc, char **argv)
{
  int r;
  options opts(argv[0]);
  fuse_operations opers;
  fuse_args args = FUSE_ARGS_INIT(argc, argv);

  fuse_opt_parse(&args, &opts, NULL, process_argument);

  if (opts.mountpoint.empty()) {
    #if defined(__APPLE__) && defined(OSX_BUNDLE)
      if (argc == 1) {
        opts.mountpoint = OSX_MOUNTPOINT_PREFIX + config::get_bucket_name();

        mkdir(opts.mountpoint.c_str(), 0777);

        fuse_opt_add_arg(&args, opts.mountpoint.c_str());
      } else {
        print_usage(opts.base_name);
        return 1;
      }
    #else
      print_usage(opts.base_name);
      return 1;
    #endif
  }

  try {
    init::base(init::IB_WITH_STATS, opts.verbosity, opts.config);
    init::services();
    init::fs();

    operations::init(opts.mountpoint);
    operations::build_fuse_operations(&opers);

    assert(opers.init == NULL);

    // not an actual FS operation
    opers.init = init;

    add_missing_options(&opts, &args);

    S3_LOG(LOG_INFO, "main", "%s version %s, initialized\n", PACKAGE_NAME, PACKAGE_VERSION_WITH_REV);

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while initializing: %s\n", e.what());
    return 1;
  }

  r = fuse_main(args.argc, args.argv, &opers, NULL);

  fuse_opt_free_args(&args);

  try {
    init::cleanup();

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while cleaning up: %s\n", e.what());
  }

  return r;
}
