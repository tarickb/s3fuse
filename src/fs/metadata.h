/*
 * fs/metadata.h
 * -------------------------------------------------------------------------
 * Shared filesystem metadata keys.
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

#ifndef S3_FS_METADATA_H
#define S3_FS_METADATA_H

namespace s3
{
  namespace fs
  {
    class metadata
    {
    public:
      static const char *RESERVED_PREFIX;
      static const char *XATTR_PREFIX;

      static const char *LAST_UPDATE_ETAG;
      static const char *MODE;
      static const char *UID;
      static const char *GID;
      static const char *CREATED_TIME;
      static const char *LAST_MODIFIED_TIME;

      static const char *SHA256;
      static const char *ENC_IV;
      static const char *ENC_METADATA;
    };
  }
}

#endif
