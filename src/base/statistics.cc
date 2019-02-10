/*
 * base/statistics.cc
 * -------------------------------------------------------------------------
 * Definitions for statistics-collection methods.
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

#include "base/statistics.h"

namespace s3 {
namespace base {

namespace {
std::mutex s_mutex;
std::unique_ptr<std::ostream> s_stream;

void LogWrite(std::string id, const char *format, va_list args) {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (!s_stream) return;

  constexpr size_t BUF_LEN = 256;
  char buf[BUF_LEN];
  vsnprintf(buf, BUF_LEN, format, args);
  (*s_stream) << id << ": " << buf << std::endl;
}
}  // namespace

void Statistics::Init(std::unique_ptr<std::ostream> output) {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (s_stream)
    throw std::runtime_error("can't call statistics::init() more than once!");
  s_stream = std::move(output);
}

void Statistics::Init(std::string file) {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (s_stream)
    throw std::runtime_error("can't call statistics::init() more than once!");
  std::unique_ptr<std::ofstream> f(new std::ofstream());
  f->open(Paths::Transform(file).c_str(), std::ofstream::trunc);
  if (!f->good())
    throw std::runtime_error("cannot open statistics target file for write");
  s_stream = std::move(f);
}

void Statistics::Collect() {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (!s_stream) return;
  for (auto iter = Writers::begin(); iter != Writers::end(); ++iter) {
    iter->second(s_stream.get());
  }
}

void Statistics::Flush() {
  if (s_stream) s_stream->flush();
}

void Statistics::Write(std::string id, const char *format, ...) {
  va_list args;

  va_start(args, format);
  LogWrite(id, format, args);
  va_end(args);
}

void Statistics::Write(std::string id, std::string tag, const char *format,
                       ...) {
  va_list args;

  va_start(args, format);
  LogWrite(id + "_" + tag, format, args);
  va_end(args);
}

}  // namespace base
}  // namespace s3
