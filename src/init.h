/*
 * init.h
 * -------------------------------------------------------------------------
 * Initialization helper class declaration.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#ifndef S3_INIT_H
#define S3_INIT_H

#include <string>

#include "base/logger.h"

namespace s3
{
  class init
  {
  public:
    enum base_flags
    {
      IB_NONE       = 0x0,
      IB_WITH_STATS = 0x1
    };

    static void base(int flags = IB_NONE, int verbosity = LOG_WARNING, const std::string &config_file = "");
    static void fs();
    static void services();
    static void threads();

    static std::string get_enabled_services();

    static void cleanup();
  };
}

#endif
