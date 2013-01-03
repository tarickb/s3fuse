/*
 * base/statistics.h
 * -------------------------------------------------------------------------
 * Statistics collection and output.
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

#ifndef S3_BASE_STATISTICS_H
#define S3_BASE_STATISTICS_H

#include <stdarg.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>

namespace s3
{
  namespace base
  {
    class statistics
    {
    public:
      static void init();

      inline static void write(const std::string &target = "")
      {
        if (target.empty()) {
          write(&std::cout);
        } else {
          std::ofstream f;

          f.open(target.c_str(), std::ofstream::trunc);

          if (!f.good())
            throw std::runtime_error("cannot open statistics target file for write");

          write(&f);

          f.close();
        }
      }

      static void write(std::ostream *output);

      template <class T>
      inline static void post(const std::string &id, T tag, const char *format, ...)
      {
        const size_t BUF_LEN = 256;

        va_list args;
        char buf[BUF_LEN];

        va_start(args, format);
        vsnprintf(buf, BUF_LEN, format, args);
        va_end(args);

        post(id + "_" + boost::lexical_cast<std::string>(tag), buf);
      }

    private:
      static void post(const std::string &id, const std::string &stats);
    };
  }
}

#endif
