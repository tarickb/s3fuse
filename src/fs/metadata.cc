/*
 * fs/metadata.cc
 * -------------------------------------------------------------------------
 * Shared metadata key definitions.
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

#include "fs/metadata.h"

namespace s3 {
namespace fs {

const char Metadata::RESERVED_PREFIX[];

const char Metadata::XATTR_PREFIX[];

const char Metadata::LAST_UPDATE_ETAG[];
const char Metadata::MODE[];
const char Metadata::UID[];
const char Metadata::GID[];
const char Metadata::CREATED_TIME[];
const char Metadata::LAST_MODIFIED_TIME[];

const char Metadata::FILE_TYPE[];
const char Metadata::DEVICE[];

const char Metadata::SHA256[];
const char Metadata::ENC_IV[];
const char Metadata::ENC_METADATA[];

}  // namespace fs
}  // namespace s3
