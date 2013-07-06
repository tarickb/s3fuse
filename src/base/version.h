/*
 * base/version.h
 * -------------------------------------------------------------------------
 * Version information (set by the various configure scripts).
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

#ifndef S3_BASE_VERSION_H
#define S3_BASE_VERSION_H

namespace s3
{
  namespace base
  {
    const char *APP_NAME = PACKAGE_NAME;
    const char *APP_VERSION = PACKAGE_VERSION_WITH_REV;
    const char *APP_DESCRIPTION = "FUSE driver for cloud object storage services";
  }
}

#endif
