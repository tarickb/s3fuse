/*
 * services/utils.cc
 * -------------------------------------------------------------------------
 * Common service implementation utilities.
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

#include "services/utils.h"

#include <atomic>

#include "base/request.h"
#include "base/statistics.h"
#include "base/xml.h"

namespace s3 {
namespace services {

namespace {
constexpr char REQ_TIMEOUT_XPATH[] = "/Error/Code[text() = 'RequestTimeout']";

std::atomic_int s_internal_server_error(0), s_service_unavailable(0);
std::atomic_int s_req_timeout(0), s_bad_request(0);

void StatsWriter(std::ostream *o) {
  *o << "common service base:\n"
        "  \"internal server error\": "
     << s_internal_server_error
     << "\n"
        "  \"service unavailable\": "
     << s_service_unavailable
     << "\n"
        "  \"RequestTimeout\": "
     << s_req_timeout
     << "\n"
        "  \"bad request\": "
     << s_bad_request << "\n";
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

bool GenericShouldRetry(base::Request *r, int iter) {
  int rc = r->response_code();

  if (rc == base::HTTP_SC_INTERNAL_SERVER_ERROR) {
    ++s_internal_server_error;
    return true;
  }

  if (rc == base::HTTP_SC_SERVICE_UNAVAILABLE) {
    ++s_service_unavailable;
    return true;
  }

  if (rc == base::HTTP_SC_BAD_REQUEST) {
    try {
      auto xml = base::XmlDocument::Parse(r->GetOutputAsString());
      if (xml && xml->Match(REQ_TIMEOUT_XPATH)) {
        ++s_req_timeout;
        return true;
      }
    } catch (...) {
    }

    ++s_bad_request;
    return false;
  }

  return false;
}

}  // namespace services
}  // namespace s3
