#include <iostream>
#include <string>

#include "base/lru_cache_map.h"

using std::cout;
using std::endl;
using std::string;

using s3::base::lru_cache_map;

inline bool remove_if_over_100(const int &i)
{
  return (i > 100);
}

void print_entry(const std::string &key, const int &i)
{
  cout << "    " << key << ": " << i << endl;
}

template <class T>
void print(const char *desc, const T &t)
{
  cout << desc << endl;
  
  cout << "  newest to oldest:" << endl;
  t.for_each_newest(print_entry);
  
  cout << "  oldest to newest:" << endl;
  t.for_each_oldest(print_entry);
}

int main(int argc, char **argv)
{
  lru_cache_map<string, int> c1(5);
  lru_cache_map<string, int, remove_if_over_100> c2(5);

  c1["entry 1"] = c2["entry 1"] = 1;
  c1["entry 2"] = c2["entry 2"] = 2;
  c1["entry 3"] = c2["entry 3"] = 101;
  c1["entry 4"] = c2["entry 4"] = 102;

  print("init, c1", c1);
  print("init, c2", c2);

  c1["entry 5"] = c2["entry 5"] = 200;

  print("add entry 5, c1", c1);
  print("add entry 5, c2", c2);

  c1["entry 6"] = c2["entry 6"] = 300;

  print("add entry 6, c1", c1);
  print("add entry 6, c2", c2);

  return 0;
}
