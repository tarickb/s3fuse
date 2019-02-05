/*
 * services/fvs/file_transfer.cc
 * -------------------------------------------------------------------------
 * FVS file transfer implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir, Hiroyuki Kakine.
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

#include "services/fvs/file_transfer.h"

namespace s3 {
namespace services {
namespace fvs {

size_t file_transfer::get_upload_chunk_size() {
  return 0; // disabled
}

} // namespace fvs
} // namespace services
} // namespace s3
