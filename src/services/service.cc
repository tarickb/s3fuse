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

#include <memory>
#include <stdexcept>

#include "base/request.h"
#include "base/request_hook.h"
#include "services/service.h"

namespace s3 {
namespace services {

impl::ptr service::s_impl;
std::shared_ptr<base::request_hook> service::s_hook;
std::shared_ptr<file_transfer> service::s_file_transfer;

namespace {
class service_hook : public base::request_hook {
public:
  service_hook(const impl::ptr &impl) : _impl(impl) {}

  virtual ~service_hook() {}

  virtual std::string adjust_url(const std::string &url) {
    return _impl->adjust_url(url);
  }

  virtual void pre_run(base::request *r, int iter) { _impl->pre_run(r, iter); }

  virtual bool should_retry(base::request *r, int iter) {
    return _impl->should_retry(r, iter);
  }

private:
  impl::ptr _impl;
};
} // namespace

void service::init(const impl::ptr &svc) {
  s_impl = svc;
  s_hook.reset(new service_hook(s_impl));
  s_file_transfer = s_impl->build_file_transfer();
}
} // namespace services
} // namespace s3
