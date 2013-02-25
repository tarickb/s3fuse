/*
 * services/fvs/file_transfer.h
 * -------------------------------------------------------------------------
 * FVS file transfer class declaration.
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

#ifndef S3_SERVICES_FVS_FILE_TRANSFER_H
#define S3_SERVICES_FVS_FILE_TRANSFER_H

#include <string>

#include "services/file_transfer.h"

namespace s3
{
  namespace services
  {
    namespace fvs
    {
      class file_transfer : public services::file_transfer
      {
      public:
        virtual size_t get_download_chunk_size();
        virtual size_t get_upload_chunk_size();
      };
    }
  }
}

#endif
