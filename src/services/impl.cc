/*
 * services/impl.cc
 * -------------------------------------------------------------------------
 * Service base class implementation.
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

#include <atomic>

#include "base/request.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "services/impl.h"

using std::atomic_int;
using std::ostream;

using s3::base::request;
using s3::base::statistics;
using s3::base::xml;
using s3::services::impl;

namespace
{
  const char *REQ_TIMEOUT_XPATH = "/Error/Code[text() = 'RequestTimeout']";

  atomic_int s_internal_server_error(0), s_service_unavailable(0);
  atomic_int s_req_timeout(0), s_bad_request(0);

  void statistics_writer(ostream *o)
  {
    *o <<
      "common service base:\n"
      "  \"internal server error\": " << s_internal_server_error << "\n"
      "  \"service unavailable\": " << s_service_unavailable << "\n"
      "  \"RequestTimeout\": " << s_req_timeout << "\n"
      "  \"bad request\": " << s_bad_request << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

impl::~impl()
{
}

bool impl::should_retry(request *r, int iter)
{
  long rc = r->get_response_code();

  if (rc == base::HTTP_SC_INTERNAL_SERVER_ERROR) {
    ++s_internal_server_error;
    return true;
  }

  if (rc == base::HTTP_SC_SERVICE_UNAVAILABLE) {
    ++s_service_unavailable;
    return true;
  }

  if (rc == base::HTTP_SC_BAD_REQUEST) {
    if (xml::match(r->get_output_buffer(), REQ_TIMEOUT_XPATH)) {
      ++s_req_timeout;
      return true;
    }

    ++s_bad_request;
    return false;
  }

  return false;
}
