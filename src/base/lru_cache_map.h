/*
 * base/lru_cache_map.h
 * -------------------------------------------------------------------------
 * Templated size-limited associative container with least-recently-used
 * eviction policy.
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

#ifndef S3_BASE_LRU_CACHE_MAP_H
#define S3_BASE_LRU_CACHE_MAP_H

#include <functional>
#include <map>

namespace s3 {
namespace base {
template <class T>
inline bool DefaultRemovableTest(const T &t) {
  return true;
}

template <class KeyType, class ValueType,
          bool (*IsRemovableCallback)(const ValueType &v) =
              DefaultRemovableTest<ValueType>>
class LruCacheMap {
 public:
  using IteratorCallback =
      std::function<void(const KeyType &, const ValueType &)>;

  inline explicit LruCacheMap(size_t max_size) : max_size_(max_size) {}

  inline ValueType &operator[](const KeyType &key) {
    Entry *e = &map_[key];
    if (e->valid) {
      Unlink(e);
    } else {
      e->key = key;
      e->valid = true;
      if (map_.size() > max_size_) {
        Entry *to_remove = GetRemovable();
        if (to_remove) Erase(to_remove->key);
      }
    }
    MakeNewest(e);
    return e->value;
  }

  inline void Erase(const KeyType &key) {
    auto iter = map_.find(key);
    if (iter == map_.end()) return;
    Entry *e = &iter->second;
    Unlink(e);
    map_.erase(iter);
  }

  inline void ForEachNewest(const IteratorCallback &cb) const {
    Entry *e = newest_;
    while (e) {
      cb(e->key, e->value);
      e = e->older;
    }
  }

  inline void ForEachOldest(const IteratorCallback &cb) const {
    Entry *e = oldest_;
    while (e) {
      cb(e->key, e->value);
      e = e->newer;
    }
  }

  inline size_t size() { return map_.size(); }

  inline bool Find(const KeyType &key, ValueType *t) {
    auto iter = map_.find(key);
    if (iter == map_.end()) return false;
    if (t) *t = iter->second.value;
    return true;
  }

 private:
  struct Entry {
    KeyType key;
    ValueType value;
    Entry *older = nullptr;
    Entry *newer = nullptr;
    bool valid = false;
  };

  using Map = std::map<KeyType, Entry>;

  inline void Unlink(Entry *e) {
    if (e == oldest_) oldest_ = e->newer;
    if (e == newest_) newest_ = e->older;
    if (e->older) e->older->newer = e->newer;
    if (e->newer) e->newer->older = e->older;
    e->newer = e->older = nullptr;
  }

  inline void MakeNewest(Entry *e) {
    e->older = newest_;
    if (newest_) newest_->newer = e;
    newest_ = e;
    if (!oldest_) oldest_ = e;
  }

  inline Entry *GetRemovable() {
    Entry *e = oldest_;
    while (e) {
      if (IsRemovableCallback(e->value)) break;
      e = e->newer;
    }
    return e;
  }

  Map map_;
  const size_t max_size_;
  Entry *newest_ = nullptr;
  Entry *oldest_ = nullptr;
};
}  // namespace base
}  // namespace s3

#endif
