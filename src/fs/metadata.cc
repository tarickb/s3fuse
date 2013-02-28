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

using s3::fs::metadata;

const char *metadata::RESERVED_PREFIX    = "s3fuse-";

// this can't share a common prefix with RESERVED_PREFIX
const char *metadata::XATTR_PREFIX       = "s3fuse_xattr_";

const char *metadata::LAST_UPDATE_ETAG   = "s3fuse-lu-etag";
const char *metadata::MODE               = "s3fuse-mode";
const char *metadata::UID                = "s3fuse-uid";
const char *metadata::GID                = "s3fuse-gid";
const char *metadata::CREATED_TIME       = "s3fuse-ctime";
const char *metadata::LAST_MODIFIED_TIME = "s3fuse-mtime";

const char *metadata::SHA256             = "s3fuse-sha256";
const char *metadata::ENC_IV             = "s3fuse-e-iv";
const char *metadata::ENC_METADATA       = "s3fuse-e-meta";
