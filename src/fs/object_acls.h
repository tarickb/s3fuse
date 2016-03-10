/*
 * fs/object_alcs.h
 * -------------------------------------------------------------------------
 * MIME type lookup declaration.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2014, Hiroyuki Kakine.
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

#ifndef S3_FS_OBJECT_ACLS_H
#define S3_FS_OBJECT_ACLS_H

#include <string>

namespace s3
{
  namespace fs
  {
    class object_acls
    {
    public:
      static void init();
      static const std::string & get_acl(std::string path);
    };
  }
}

#endif
