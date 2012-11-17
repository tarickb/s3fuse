#include <iostream>
#include <string>
#include <boost/bind.hpp>

#include "base/lru_cache_map.h"

using boost::bind;
using std::cout;
using std::endl;
using std::string;

using s3::base::lru_cache_map;

inline bool remove_if_over_100(const int &i)
{
  return (i > 100);
}

void append_to_string(const string &key, const int &i, string *str)
{
  *str += (str->empty() ? "" : ",");
  *str += key;
}

template <class T>
void test(const char *desc, const T &t, const char *expected_newest, const char *expected_oldest)
{
  string newest, oldest;

  t.for_each_newest(bind(append_to_string, _1, _2, &newest));
  t.for_each_oldest(bind(append_to_string, _1, _2, &oldest));

  cout << desc << " newest: " << newest << endl;
  cout << desc << " oldest: " << oldest << endl;

  if (newest != expected_newest)
    cout << "FAILED: " << desc << " newest: [" << newest << "], expected: [" << expected_newest << "]" << endl;

  if (oldest != expected_oldest)
    cout << "FAILED: " << desc << " oldest: [" << oldest << "], expected: [" << expected_oldest << "]" << endl;
}

int main(int argc, char **argv)
{
  lru_cache_map<string, int> c1(5);
  lru_cache_map<string, int, remove_if_over_100> c2(5);

  c1["e1"] = c2["e1"] = 1;
  c1["e2"] = c2["e2"] = 2;
  c1["e3"] = c2["e3"] = 101;
  c1["e4"] = c2["e4"] = 102;

  test("init, c1", c1, "e4,e3,e2,e1", "e1,e2,e3,e4");
  test("init, c2", c2, "e4,e3,e2,e1", "e1,e2,e3,e4");

  c1["e5"] = c2["e5"] = 200;

  test("add e5, c1", c1, "e5,e4,e3,e2,e1", "e1,e2,e3,e4,e5");
  test("add e5, c2", c2, "e5,e4,e3,e2,e1", "e1,e2,e3,e4,e5");

  c1["e6"] = c2["e6"] = 300;

  test("add e6, c1", c1, "e6,e5,e4,e3,e2", "e2,e3,e4,e5,e6");
  test("add e6, c2", c2, "e6,e5,e4,e2,e1", "e1,e2,e4,e5,e6");

  cout << "c1 value of e2: " << c1["e2"] << endl;
  cout << "c2 value of e2: " << c2["e2"] << endl;

  test("get e2, c1", c1, "e2,e6,e5,e4,e3", "e3,e4,e5,e6,e2");
  test("get e2, c2", c2, "e2,e6,e5,e4,e1", "e1,e4,e5,e6,e2");

  c1["e7"] = c2["e7"] = 400;

  test("add e7, c1", c1, "e7,e2,e6,e5,e4", "e4,e5,e6,e2,e7");
  test("add e7, c2", c2, "e7,e2,e6,e5,e1", "e1,e5,e6,e2,e7");

  c1.erase("e1");
  c2.erase("e1");

  test("erase e1, c1", c1, "e7,e2,e6,e5,e4", "e4,e5,e6,e2,e7");
  test("erase e1, c2", c2, "e7,e2,e6,e5", "e5,e6,e2,e7");

  c1.erase("e2");
  c2.erase("e2");

  test("erase e2, c1", c1, "e7,e6,e5,e4", "e4,e5,e6,e7");
  test("erase e2, c2", c2, "e7,e6,e5", "e5,e6,e7");

  cout << "c1 value of e7: " << c1["e7"] << endl;
  cout << "c2 value of e7: " << c2["e7"] << endl;

  test("get e7, c1", c1, "e7,e6,e5,e4", "e4,e5,e6,e7");
  test("get e7, c2", c2, "e7,e6,e5", "e5,e6,e7");

  cout << "c1 value of e5: " << c1["e5"] << endl;
  cout << "c2 value of e5: " << c2["e5"] << endl;

  test("get e5, c1", c1, "e5,e7,e6,e4", "e4,e6,e7,e5");
  test("get e5, c2", c2, "e5,e7,e6", "e6,e7,e5");

  c1["e8"] = c2["e8"] = 500;

  test("add e8, c1", c1, "e8,e5,e7,e6,e4", "e4,e6,e7,e5,e8");
  test("add e8, c2", c2, "e8,e5,e7,e6", "e6,e7,e5,e8");

  c1["e1"] = c2["e1"] = 600;

  test("re-add e1, c1", c1, "e1,e8,e5,e7,e6", "e6,e7,e5,e8,e1");
  test("re-add e1, c2", c2, "e1,e8,e5,e7,e6", "e6,e7,e5,e8,e1");

  return 0;
}
