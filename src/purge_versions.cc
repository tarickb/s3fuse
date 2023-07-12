/*
 * purge_versions.cc
 * -------------------------------------------------------------------------
 * Purge all versions of all objects with a given prefix.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2019, Tarick Bedeir.
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

#include <getopt.h>

#include <cstring>
#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/xml.h"
#include "fs/list_reader.h"
#include "fs/object.h"
#include "services/service.h"
#include "services/versioning.h"

namespace {

void DeleteVersions(s3::base::Request *req, const std::string &key,
                    bool dry_run) {
  std::list<s3::services::ObjectVersion> versions;
  if (s3::services::Service::versioning()->FetchAllVersions(
          s3::services::VersionFetchOptions::ALL, key, req, &versions,
          nullptr) != 0)
    throw std::runtime_error("failed to fetch versions.");

  for (const auto &version : versions) {
    std::cout << "    delete version: " << version.version << std::endl;
    const std::string url =
        s3::services::Service::versioning()->BuildVersionedUrl(key,
                                                               version.version);
    if (!dry_run) {
      if (s3::fs::Object::RemoveByUrl(req, url) != 0)
        throw std::runtime_error("delete request failed");
    }
  }
}

void PrintUsage(const char *arg0) {
  const char *base_name = std::strrchr(arg0, '/');
  base_name = base_name ? base_name + 1 : arg0;
  std::cerr << "Usage: " << base_name
            << " -c <config-file> [ -n ] <prefix>\n"
               "\n"
               "See "
            << base_name << "(1) for examples and a more detailed explanation."
            << std::endl;
  exit(1);
}

}  // namespace

int main(int argc, char **argv) {
  int opt = 0;
  std::string config_file;
  bool dry_run = false;
  while ((opt = getopt(argc, argv, "c:n")) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;

      case 'n':
        dry_run = true;
        break;

      default:
        PrintUsage(argv[0]);
    }
  }

  const std::string initial_prefix = (optind < argc) ? argv[optind++] : "";
  if (initial_prefix.empty()) PrintUsage(argv[0]);

  s3::base::Logger::Init(s3::base::Logger::Mode::STDERR, LOG_WARNING);
  s3::base::Config::Init(config_file);
  s3::base::XmlDocument::Init();
  s3::services::Service::Init();

  auto request = s3::base::RequestFactory::New();

  std::list<std::string> prefixes;
  prefixes.push_back(initial_prefix);

  while (!prefixes.empty()) {
    const std::string prefix = prefixes.front();
    prefixes.pop_front();

    auto reader = s3::fs::ListReader::Create(prefix);
    std::list<std::string> keys;
    std::list<std::string> new_prefixes;

    std::cout << "prefix: [" << prefix << "]" << std::endl;
    while (true) {
      int r = reader->Read(request.get(), &keys, &new_prefixes);
      if (r == 0) break;
      if (r < 0) throw std::runtime_error("failed to list bucket objects.");

      for (const auto &key : keys) {
        std::cout << "  key: [" << key << "]" << std::endl;
        DeleteVersions(request.get(), key, dry_run);
      }

      prefixes.insert(prefixes.begin(), new_prefixes.begin(),
                      new_prefixes.end());
    }
  }

  return 0;
}
