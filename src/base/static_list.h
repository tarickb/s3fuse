/*
 * base/static_list.h
 * -------------------------------------------------------------------------
 * Templated priority queue-style container that allows for load-time
 * initialization with the help of a static registration class.
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

#ifndef S3_BASE_STATIC_LIST_H
#define S3_BASE_STATIC_LIST_H

#include <map>

namespace s3
{
  namespace base
  {
    template <class T>
    class static_list
    {
    private:
      typedef typename std::multimap<int, T> priority_map;

    public:
      typedef typename priority_map::const_iterator const_iterator;

      class entry
      {
      public:
        inline entry(const T &t, int priority)
        {
          add(t, priority);
        }
      };

      inline static const_iterator begin() { return s_list->begin(); }
      inline static const_iterator end() { return s_list->end(); }

    private:
      inline static void add(const T &t, int priority)
      {
        if (!s_list)
          s_list = new priority_map;

        s_list->insert(std::make_pair(priority, t));
      }

      static priority_map *s_list;
    };

    template <class T>
    typename static_list<T>::priority_map *static_list<T>::s_list = NULL;
  }
}

#endif
