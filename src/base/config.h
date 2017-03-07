/*
 * base/config.h
 * -------------------------------------------------------------------------
 * Methods to get cached configuration values.  Mostly auto-generated with
 * configuration keys in config.inc.
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

#ifndef S3_BASE_CONFIG_H
#define S3_BASE_CONFIG_H

#include <limits.h>
#include <unistd.h>

#include <string>

#if !defined(UID_MAX)
#define UID_MAX std::numeric_limits<uint32_t>::max()
#endif

#if !defined(GID_MAX)
#define GID_MAX std::numeric_limits<uint32_t>::max()
#endif

namespace s3
{
  namespace base
  {
    class config
    {
    public:
      static void init(const std::string &file = "");

      #define CONFIG(type, name, def, desc) \
        private: \
          static type s_ ## name; \
        \
        public: \
          inline static const type & get_ ## name () { return s_ ## name; }

      #define CONFIG_CONSTRAINT(x, y)
      #define CONFIG_KEY(x)
      #define CONFIG_SECTION(x)

      #include "base/config.inc"

      #undef CONFIG
      #undef CONFIG_CONSTRAINT
      #undef CONFIG_KEY
      #undef CONFIG_SECTION
    };
  }
}

#endif
