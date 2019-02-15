/*
 * services/utils.h
 * -------------------------------------------------------------------------
 * Utility functions for service implementations.
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

#ifndef S3_SERVICES_UTILS_H
#define S3_SERVICES_UTILS_H

#include <map>

namespace s3 {
namespace base {
class Request;
}
namespace services {

template <class K, class V>
const V &FindOrDefault(const std::map<K, V> &map, K key) {
  static const auto DEFAULT = V();
  const auto iter = map.find(key);
  return (iter == map.end()) ? DEFAULT : iter->second;
}

template <class V>
const V &FindOrDefault(const std::map<std::string, V> &map, const char *key) {
  return FindOrDefault<std::string, V>(map, std::string(key));
}

bool GenericShouldRetry(base::Request *r, int iter);

}  // namespace services
}  // namespace s3

#endif
