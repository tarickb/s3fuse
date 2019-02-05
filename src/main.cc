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

#include <cstring>
#include <iostream>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "init.h"
#include "operations.h"
#include "threads/pool.h"

namespace {
const int DEFAULT_VERBOSITY = LOG_WARNING;
const char *APP_DESCRIPTION = "FUSE driver for cloud object storage services";

#ifdef __APPLE__
const std::string OSX_MOUNTPOINT_PREFIX = "/volumes/" PACKAGE_NAME "_";
#endif

struct options {
  const char *base_name;
  std::string config;
  std::string mountpoint;
  int verbosity;

#ifdef __APPLE__
  std::string volname;

  bool noappledouble_set;
  bool daemon_timeout_set;
  bool volname_set;
#endif

  options(const char *arg0) {
    verbosity = DEFAULT_VERBOSITY;

    base_name = std::strrchr(arg0, '/');
    base_name = base_name ? base_name + 1 : arg0;

#ifdef __APPLE__
    noappledouble_set = false;
    daemon_timeout_set = false;
    volname_set = false;
#endif
  }
};
} // namespace

int print_usage(const char *base_name) {
  std::cerr
      << "Usage: " << base_name
      << " [options] <mountpoint>\n"
         "\n"
         "Options:\n"
         "  -f                   stay in the foreground (i.e., do not "
         "daemonize)\n"
         "  -h, --help           print this help message and exit\n"
         "  -o OPT...            pass OPT (comma-separated) to FUSE, such as:\n"
         "     allow_other         allow other users to access the mounted "
         "file system\n"
         "     allow_root          allow root to access the mounted file "
         "system\n"
         "     default_permissions enforce permissions (useful in multiuser "
         "scenarios)\n"
         "     gid=<id>            force group ID for all files to <id>\n"
         "     config=<file>       use <file> rather than the default "
         "configuration file\n"
         "     uid=<id>            force user ID for all files to <id>\n"
         "  -v, --verbose        enable logging to stderr (can be repeated for "
         "more verbosity)\n"
         "  -vN, --verbose=N     set verbosity to N\n"
         "  -V, --version        print version and exit\n"
      << std::endl;

  return 0;
}

int print_version() {
  std::cout << PACKAGE_NAME << ", " << PACKAGE_VERSION_WITH_REV << ", "
            << APP_DESCRIPTION << std::endl;
  std::cout << "enabled services: " << s3::init::get_enabled_services()
            << std::endl;

  return 0;
}

int process_argument(void *data, const char *c_arg, int key,
                     struct fuse_args *out_args) {
  options *opts = static_cast<options *>(data);
  const std::string arg = c_arg;

  if (arg == "-V" || arg == "--version") {
    print_version();
    exit(0);
  }

  if (arg == "-h" || arg == "--help") {
    print_usage(opts->base_name);
    exit(1);
  }

  if (arg == "-v" || arg == "--verbose") {
    opts->verbosity++;
    return 0;
  }

  if (arg.find("-v") == 0) {
    opts->verbosity = std::stoi(arg.substr(std::string("-v").length()));
    return 0;
  }

  if (arg.find("--verbose=") == 0) {
    opts->verbosity = std::stoi(arg.substr(std::string("--verbose=").length()));
    return 0;
  }

  if (arg.find("config=") == 0) {
    opts->config = arg.substr(std::string("config=").length());
    return 0;
  }

#ifdef __APPLE__
  if (arg.find("daemon_timeout=") == 0) {
    opts->daemon_timeout_set = true;
    return 1; // continue processing
  }

  if (arg.find("noappledouble") == 0) {
    opts->noappledouble_set = true;
    return 1; // continue processing
  }

  if (arg.find("volname=") == 0) {
    opts->volname_set = true;
    return 1; // continue processing
  }
#endif

  if (key == FUSE_OPT_KEY_NONOPT)
    opts->mountpoint =
        c_arg; // assume that the mountpoint is the only non-option

  return 1;
}

void *init(fuse_conn_info *info) {
  if (info->capable & FUSE_CAP_ATOMIC_O_TRUNC) {
    info->want |= FUSE_CAP_ATOMIC_O_TRUNC;
    S3_LOG(LOG_DEBUG, "operations::init", "enabling FUSE_CAP_ATOMIC_O_TRUNC\n");
  } else {
    S3_LOG(LOG_WARNING, "operations::init",
           "FUSE_CAP_ATOMIC_O_TRUNC not supported, will revert to "
           "truncate-then-open\n");
  }

  // this has to be here, rather than in main(), because the threads created
  // won't survive the fork in fuse_main().
  s3::init::threads();

  return NULL;
}

void add_missing_options(options *opts, fuse_args *args) {
#ifdef __APPLE__
  opts->volname = "-ovolname=" PACKAGE_NAME " volume (" +
                  s3::base::config::get_bucket_name() + ")";

  if (!opts->daemon_timeout_set)
    fuse_opt_add_arg(args, "-odaemon_timeout=3600");

  if (!opts->noappledouble_set)
    fuse_opt_add_arg(args, "-onoappledouble");

  if (!opts->volname_set)
    fuse_opt_add_arg(args, opts->volname.c_str());
#endif
}

int main(int argc, char **argv) {
  int r;
  options opts(argv[0]);
  fuse_operations opers;
  fuse_args args = FUSE_ARGS_INIT(argc, argv);

  fuse_opt_parse(&args, &opts, NULL, process_argument);

  if (opts.mountpoint.empty()) {
#if defined(__APPLE__) && defined(OSX_BUNDLE)
    if (argc == 1) {
      opts.mountpoint =
          OSX_MOUNTPOINT_PREFIX + s3::base::config::get_bucket_name();

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
    s3::init::base(s3::init::IB_WITH_STATS, opts.verbosity, opts.config);
    s3::init::services();
    s3::init::fs();

    s3::operations::init(opts.mountpoint);
    s3::operations::build_fuse_operations(&opers);

    if (opers.init != NULL)
      throw std::runtime_error(
          "operations struct defined init when it shouldn't have.");

    // not an actual FS operation
    opers.init = init;

    add_missing_options(&opts, &args);

    S3_LOG(LOG_INFO, "main", "%s version %s, initialized\n", PACKAGE_NAME,
           PACKAGE_VERSION_WITH_REV);

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while initializing: %s\n",
           e.what());
    return 1;
  }

  r = fuse_main(args.argc, args.argv, &opers, NULL);

  fuse_opt_free_args(&args);

  try {
    s3::threads::pool::terminate();

    // these won't do anything if statistics::init() wasn't called
    s3::base::statistics::collect();
    s3::base::statistics::flush();

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "main", "caught exception while cleaning up: %s\n",
           e.what());
  }

  return r;
}
