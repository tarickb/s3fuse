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
