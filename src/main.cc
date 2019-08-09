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
#include "operations.h"
#include "services/service.h"
#include "threads/pool.h"

namespace {
struct Options {
  const char *base_name = nullptr;
  std::string config;
  std::string mountpoint;
  int verbosity = LOG_WARNING;
  bool uid_set = false;
  bool gid_set = false;
  bool foreground = false;

#ifdef __APPLE__
  std::string volname;

  bool noappledouble_set = false;
  bool daemon_timeout_set = false;
  bool volname_set = false;
#endif

  explicit Options(const char *arg0) {
    base_name = std::strrchr(arg0, '/');
    base_name = base_name ? base_name + 1 : arg0;
  }
};

void TestBucketAccess() {
  constexpr int BUCKET_TEST_MAX_RETRIES = 3;
  constexpr int BUCKET_TEST_ID_LEN = 16;

  auto req = s3::base::RequestFactory::New();
  s3::fs::ListReader reader("/", false, 1);

  std::list<std::string> keys;
  int r = reader.Read(req.get(), &keys, nullptr);
  if (r)
    throw std::runtime_error(
        "unable to list bocket contents. check bucket name and credentials.");

  int retry_count = 0;
  while (retry_count++ < BUCKET_TEST_MAX_RETRIES) {
    const auto rand_url = s3::fs::Object::BuildInternalUrl(
        std::string("bucket_test_") +
        s3::crypto::Buffer::Generate(BUCKET_TEST_ID_LEN).ToHexString());

    req->Init(s3::base::HttpMethod::HEAD);
    req->SetUrl(rand_url);
    req->Run();

    if (req->response_code() != s3::base::HTTP_SC_NOT_FOUND) {
      S3_LOG(LOG_DEBUG, "::TestBucketAccess",
             "test key exists. that's unusual.\n");
      continue;
    }

    req->Init(s3::base::HttpMethod::PUT);
    req->SetUrl(rand_url);
    req->SetInputBuffer("this is a test.");
    req->Run();

    if (req->response_code() != s3::base::HTTP_SC_OK) {
      S3_LOG(LOG_WARNING, "::TestBucketAccess",
             "cannot commit test object to bucket. access to this bucket is "
             "likely read-only. continuing anyway, but check permissions if "
             "this is unexpected.\n");
    } else {
      req->Init(s3::base::HttpMethod::DELETE);
      req->SetUrl(rand_url);
      req->Run();

      if (req->response_code() != s3::base::HTTP_SC_NO_CONTENT)
        S3_LOG(LOG_WARNING, "::TestBucketAccess",
               "unable to clean up test object. might be missing permission to "
               "delete objects. continuing anyway.\n");
    }

    return;
  }

  throw std::runtime_error("unable to complete bucket access test.");
}

int PrintUsage(const char *base_name) {
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

int PrintVersion() {
  std::cout << PACKAGE_NAME << ", " << PACKAGE_VERSION_WITH_REV << ", "
            << "FUSE driver for cloud object storage services" << std::endl;
  std::cout << "enabled services: "
            << s3::services::Service::GetEnabledServices() << std::endl;
  return 0;
}

int ProcessArgument(void *data, const char *c_arg, int key,
                    struct fuse_args *out_args) {
  auto *opts = static_cast<Options *>(data);
  const std::string arg = c_arg;

  if (arg == "-V" || arg == "--version") {
    PrintVersion();
    exit(0);
  }

  if (arg == "-h" || arg == "--help") {
    PrintUsage(opts->base_name);
    exit(1);
  }

  if (arg == "-v" || arg == "--verbose") {
    opts->verbosity++;
    return 0;
  }

  if (arg == "-f") {
    opts->foreground = true;
    return 1;  // continue processing
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

  if (arg.find("uid=") == 0) {
    opts->uid_set = true;
    return 1;  // continue processing
  }

  if (arg.find("gid=") == 0) {
    opts->gid_set = true;
    return 1;  // continue processing
  }

#ifdef __APPLE__
  if (arg.find("daemon_timeout=") == 0) {
    opts->daemon_timeout_set = true;
    return 1;  // continue processing
  }

  if (arg.find("noappledouble") == 0) {
    opts->noappledouble_set = true;
    return 1;  // continue processing
  }

  if (arg.find("volname=") == 0) {
    opts->volname_set = true;
    return 1;  // continue processing
  }
#endif

  if (key == FUSE_OPT_KEY_NONOPT)
    opts->mountpoint =
        c_arg;  // assume that the mountpoint is the only non-option

  return 1;
}

void *Init(fuse_conn_info *info) {
  if (info->capable & FUSE_CAP_ATOMIC_O_TRUNC) {
    info->want |= FUSE_CAP_ATOMIC_O_TRUNC;
    S3_LOG(LOG_DEBUG, "::Init", "enabling FUSE_CAP_ATOMIC_O_TRUNC\n");
  } else {
    S3_LOG(LOG_WARNING, "::Init",
           "FUSE_CAP_ATOMIC_O_TRUNC not supported, will revert to "
           "truncate-then-open\n");
  }

  // this has to be here, rather than in main(), because the threads created
  // won't survive the fork in fuse_main().
  s3::threads::Pool::Init();

  return nullptr;
}

void AddMissingOptions(Options *opts, fuse_args *args) {
#ifdef __APPLE__
#ifdef OSX_BUNDLE
  constexpr char DEFAULT_MOUNTPOINT_PREFIX[] =
      "~/.s3fuse/volume_" PACKAGE_NAME "_";
  if (opts->mountpoint.empty()) {
    opts->mountpoint = s3::base::Paths::Transform(DEFAULT_MOUNTPOINT_PREFIX) +
                       s3::base::Config::bucket_name();
    mkdir(opts->mountpoint.c_str(), 0777);
    S3_LOG(LOG_INFO, "main", "Using default mountpoint: %s\n",
           opts->mountpoint.c_str());
    fuse_opt_add_arg(args, opts->mountpoint.c_str());
  }
#endif
  opts->volname = "-ovolname=" PACKAGE_NAME " volume (" +
                  s3::base::Config::bucket_name() + ")";
  if (!opts->daemon_timeout_set)
    fuse_opt_add_arg(args, "-odaemon_timeout=3600");
  if (!opts->noappledouble_set) fuse_opt_add_arg(args, "-onoappledouble");
  if (!opts->volname_set) fuse_opt_add_arg(args, opts->volname.c_str());
#endif
  if (s3::base::Config::ignore_object_uid_gid()) {
    if (!opts->uid_set) {
      std::string uid_str = "-ouid=" + std::to_string(getuid());
      fuse_opt_add_arg(args, strdup(uid_str.c_str()));
    }
    if (!opts->gid_set) {
      std::string gid_str = "-ogid=" + std::to_string(getgid());
      fuse_opt_add_arg(args, strdup(gid_str.c_str()));
    }
  }
  if (s3::base::Config::mount_readonly()) {
    fuse_opt_add_arg(args, "-oro");
  }
}
}  // namespace

int main(int argc, char **argv) {
  Options opts(argv[0]);
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  fuse_opt_parse(&args, &opts, nullptr, ProcessArgument);

  if (opts.mountpoint.empty()) {
#if defined(__APPLE__) && defined(OSX_BUNDLE)
    if (argc != 1) {
      PrintUsage(opts.base_name);
      return 1;
    }
#else
    PrintUsage(opts.base_name);
    return 1;
#endif
  }

  fuse_operations opers;

  try {
    auto log_mode = opts.foreground ? s3::base::Logger::Mode::STDERR
                                    : s3::base::Logger::Mode::SYSLOG;
    s3::base::Logger::Init(log_mode, opts.verbosity);
    s3::base::Config::Init(opts.config);
    s3::base::XmlDocument::Init();

    if (!s3::base::Config::stats_file().empty())
      s3::base::Statistics::Init(s3::base::Config::stats_file());

    s3::services::Service::Init();

    s3::fs::File::TestTransferChunkSizes();

    s3::fs::Cache::Init();
    s3::fs::Encryption::Init();
    s3::fs::MimeTypes::Init();

    TestBucketAccess();

    AddMissingOptions(&opts, &args);

    s3::Operations::Init(opts.mountpoint);
    s3::Operations::BuildFuseOperations(&opers);
    if (opers.init != nullptr)
      throw std::runtime_error(
          "operations struct defined init when it shouldn't have.");
    // not an actual FS operation
    opers.init = Init;

    S3_LOG(LOG_INFO, "::main", "%s version %s, initialized\n", PACKAGE_NAME,
           PACKAGE_VERSION_WITH_REV);
  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "::main", "caught exception while initializing: %s\n",
           e.what());
    return 1;
  }

  int r = fuse_main(args.argc, args.argv, &opers, nullptr);

  fuse_opt_free_args(&args);
  try {
    s3::threads::Pool::Terminate();
    // these won't do anything if statistics::init() wasn't called
    s3::base::Statistics::Collect();
    s3::base::Statistics::Flush();
  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "::main", "caught exception while cleaning up: %s\n",
           e.what());
  }

  return r;
}
