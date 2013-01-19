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

#include <stdexcept>
#include <boost/detail/atomic_count.hpp>

#include "base/request.h"
#include "base/request_hook.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "services/service.h"
#include "services/aws_impl.h"
#include "services/gs_impl.h"

using boost::shared_ptr;
using boost::detail::atomic_count;
using std::ostream;
using std::runtime_error;
using std::string;

using s3::base::request;
using s3::base::request_hook;
using s3::base::statistics;
using s3::base::xml;
using s3::services::aws_impl;
using s3::services::gs_impl;
using s3::services::impl;
using s3::services::service;

impl::ptr service::s_impl;
shared_ptr<request_hook> service::s_hook;

namespace
{
  const char *REQ_TIMEOUT_XPATH = "/Error/Code[text() = 'RequestTimeout']";

  atomic_count s_req_timeout(0), s_bad_request(0);

  class service_hook : public request_hook
  {
  public:
    service_hook(const impl::ptr &impl)
      : _impl(impl)
    {
    }

    virtual ~service_hook()
    {
    }

    virtual string adjust_url(const string &url)
    {
      return _impl->adjust_url(url);
    }

    virtual void pre_run(request *r, int iter)
    {
      _impl->pre_run(r, iter);
    }

    virtual bool should_retry(request *r, int iter)
    {
      long rc = r->get_response_code();

      if (rc == s3::base::HTTP_SC_BAD_REQUEST) {
        if (xml::match(r->get_output_buffer(), REQ_TIMEOUT_XPATH)) {
          // TODO: refresh connection?

          ++s_req_timeout;
          return true;
        }

        ++s_bad_request;
        return false;
      }

      return _impl->should_retry(r, iter);
    }

  private:
    impl::ptr _impl;
  };

  void statistics_writer(ostream *o)
  {
    *o <<
      "service_hook:\n"
      "  \"RequestTimeout\": " << s_req_timeout << "\n"
      "  \"bad request\": " << s_bad_request << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

void service::init(const string &service)
{
  if (service == "aws")
    s_impl.reset(new aws_impl());

  else if (service == "google-storage")
    s_impl.reset(new gs_impl());

  else
    throw runtime_error("unrecognized service.");

  s_hook.reset(new service_hook(s_impl));
}
