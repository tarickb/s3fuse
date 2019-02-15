/*
 * services/service.cc
 * -------------------------------------------------------------------------
 * Implementation for service static methods.
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

#include "services/service.h"

#include <memory>
#include <stdexcept>

#include "base/config.h"
#include "base/request.h"
#include "base/request_hook.h"
#include "services/file_transfer.h"
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
namespace services {
std::unique_ptr<Impl> Service::s_impl;

void Service::Init() {
#ifdef FIXED_SERVICE
  Init(std::unique_ptr<Impl>(new services::FIXED_SERVICE::Impl()));
  return;
#else
#define TEST_SVC(str, ns)                                    \
  do {                                                       \
    if (base::Config::service() == str) {                    \
      Init(std::unique_ptr<Impl>(new services::ns::Impl())); \
      return;                                                \
    }                                                        \
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

void Service::Init(std::unique_ptr<Impl> impl) {
  s_impl = std::move(impl);
  base::RequestFactory::SetHook(s_impl->hook());
}

std::string Service::GetEnabledServices() {
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
                      : svcs.substr(2);  // strip leading ", "
}
}  // namespace services
}  // namespace s3
