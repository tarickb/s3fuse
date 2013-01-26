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
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "base/paths.h"
#include "base/static_list.h"

namespace s3
{
  namespace base
  {
    class statistics
    {
    public:
      typedef boost::function1<void, std::ostream *> writer_fn;

      typedef static_list<writer_fn> writers;

      inline static void init(const boost::shared_ptr<std::ostream> &output)
      {
        boost::mutex::scoped_lock lock(s_mutex);

        if (s_stream)
          throw std::runtime_error("can't call statistics::init() more than once!");

        s_stream = output;
      }

      inline static void init(const std::string &output_file)
      {
        boost::shared_ptr<std::ofstream> f(new std::ofstream());

        f->open(paths::transform(output_file).c_str(), std::ofstream::trunc);

        if (!f->good())
          throw std::runtime_error("cannot open statistics target file for write");

        init(f);
      }

      inline static void collect()
      {
        boost::mutex::scoped_lock lock(s_mutex);

        if (!s_stream)
          return;

        for (writers::const_iterator itor = writers::begin(); itor != writers::end(); ++itor)
          itor->second(s_stream.get());
      }

      inline static void flush()
      {
        if (s_stream)
          s_stream->flush();
      }

      inline static void write(const std::string &id, const char *format, ...)
      {
        va_list args;

        va_start(args, format);
        write(id, format, args);
        va_end(args);
      }

      template <class T>
      inline static void write(const std::string &id, T tag, const char *format, ...)
      {
        va_list args;

        va_start(args, format);
        write(id + "_" + boost::lexical_cast<std::string>(tag), format, args);
        va_end(args);
      }

    private:
      inline static void write(const std::string &id, const char *format, va_list args) 
      {
        const size_t BUF_LEN = 256;

        char buf[BUF_LEN];
        boost::mutex::scoped_lock lock(s_mutex);

        if (!s_stream)
          return;

        vsnprintf(buf, BUF_LEN, format, args);

        (*s_stream) << id << ": " << buf << std::endl;
      }

      static boost::mutex s_mutex;
      static boost::shared_ptr<std::ostream> s_stream;
    };
  }
}

#endif
