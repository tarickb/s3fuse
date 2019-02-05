#include <functional>
#include <gtest/gtest.h>
#include <string>

#include "base/lru_cache_map.h"

namespace s3 {
namespace base {
namespace tests {

namespace {

inline bool remove_if_over_100(const int &i) { return (i > 100); }

void append_to_string(const std::string &key, const int &i, std::string *str) {
  *str += (str->empty() ? "" : ",");
  *str += key;
}

template <class T> std::string oldest(const T &t) {
  std::string s;

  t.for_each_oldest(std::bind(append_to_string, std::placeholders::_1,
                              std::placeholders::_2, &s));

  return s;
}

template <class T> std::string newest(const T &t) {
  std::string s;

  t.for_each_newest(std::bind(append_to_string, std::placeholders::_1,
                              std::placeholders::_2, &s));

  return s;
}

} // namespace

TEST(lru_cache_map, no_remove_condition) {
  lru_cache_map<std::string, int> c(5);

  c["e1"] = 1;
  c["e2"] = 2;
  c["e3"] = 101;
  c["e4"] = 102;

  EXPECT_EQ(static_cast<size_t>(4), c.get_size());

  EXPECT_EQ(std::string("e4,e3,e2,e1"), newest(c)) << "init, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4"), oldest(c)) << "init, oldest";

  c["e5"] = 200;

  EXPECT_EQ(std::string("e5,e4,e3,e2,e1"), newest(c)) << "add e5, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4,e5"), oldest(c)) << "add e5, oldest";

  c["e6"] = 300;

  EXPECT_EQ(std::string("e6,e5,e4,e3,e2"), newest(c)) << "add e6, newest";
  EXPECT_EQ(std::string("e2,e3,e4,e5,e6"), oldest(c)) << "add e6, oldest";

  EXPECT_EQ(2, c["e2"]);

  EXPECT_EQ(std::string("e2,e6,e5,e4,e3"), newest(c)) << "get e2, newest";
  EXPECT_EQ(std::string("e3,e4,e5,e6,e2"), oldest(c)) << "get e2, oldest";

  c["e7"] = 400;

  EXPECT_EQ(std::string("e7,e2,e6,e5,e4"), newest(c)) << "add e7, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e2,e7"), oldest(c)) << "add e7, oldest";

  c.erase("e1");

  EXPECT_EQ(std::string("e7,e2,e6,e5,e4"), newest(c)) << "erase e1, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e2,e7"), oldest(c)) << "erase e1, oldest";

  c.erase("e2");

  EXPECT_EQ(std::string("e7,e6,e5,e4"), newest(c)) << "erase e2, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e7"), oldest(c)) << "erase e2, oldest";

  EXPECT_EQ(400, c["e7"]);

  EXPECT_EQ(std::string("e7,e6,e5,e4"), newest(c)) << "get e7, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e7"), oldest(c)) << "get e7, oldest";

  EXPECT_EQ(200, c["e5"]);

  EXPECT_EQ(std::string("e5,e7,e6,e4"), newest(c)) << "get e5, newest";
  EXPECT_EQ(std::string("e4,e6,e7,e5"), oldest(c)) << "get e5, oldest";

  c["e8"] = 500;

  EXPECT_EQ(std::string("e8,e5,e7,e6,e4"), newest(c)) << "add e8, newest";
  EXPECT_EQ(std::string("e4,e6,e7,e5,e8"), oldest(c)) << "add e8, oldest";

  c["e1"] = 600;

  EXPECT_EQ(std::string("e1,e8,e5,e7,e6"), newest(c)) << "re-add e1, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8,e1"), oldest(c)) << "re-add e1, oldest";
}

TEST(lru_cache_map, remove_if_over_100) {
  lru_cache_map<std::string, int, remove_if_over_100> c(5);

  c["e1"] = 1;
  c["e2"] = 2;
  c["e3"] = 101;
  c["e4"] = 102;

  EXPECT_EQ(static_cast<size_t>(4), c.get_size());

  EXPECT_EQ(std::string("e4,e3,e2,e1"), newest(c)) << "init, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4"), oldest(c)) << "init, oldest";

  c["e5"] = 200;

  EXPECT_EQ(std::string("e5,e4,e3,e2,e1"), newest(c)) << "add e5, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4,e5"), oldest(c)) << "add e5, oldest";

  c["e6"] = 300;

  EXPECT_EQ(std::string("e6,e5,e4,e2,e1"), newest(c)) << "add e6, newest";
  EXPECT_EQ(std::string("e1,e2,e4,e5,e6"), oldest(c)) << "add e6, oldest";

  EXPECT_EQ(2, c["e2"]);

  EXPECT_EQ(std::string("e2,e6,e5,e4,e1"), newest(c)) << "get e2, newest";
  EXPECT_EQ(std::string("e1,e4,e5,e6,e2"), oldest(c)) << "get e2, oldest";

  c["e7"] = 400;

  EXPECT_EQ(std::string("e7,e2,e6,e5,e1"), newest(c)) << "add e7, newest";
  EXPECT_EQ(std::string("e1,e5,e6,e2,e7"), oldest(c)) << "add e7, oldest";

  c.erase("e1");

  EXPECT_EQ(std::string("e7,e2,e6,e5"), newest(c)) << "erase e1, newest";
  EXPECT_EQ(std::string("e5,e6,e2,e7"), oldest(c)) << "erase e1, oldest";

  c.erase("e2");

  EXPECT_EQ(std::string("e7,e6,e5"), newest(c)) << "erase e2, newest";
  EXPECT_EQ(std::string("e5,e6,e7"), oldest(c)) << "erase e2, oldest";

  EXPECT_EQ(400, c["e7"]);

  EXPECT_EQ(std::string("e7,e6,e5"), newest(c)) << "get e7, newest";
  EXPECT_EQ(std::string("e5,e6,e7"), oldest(c)) << "get e7, oldest";

  EXPECT_EQ(200, c["e5"]);

  EXPECT_EQ(std::string("e5,e7,e6"), newest(c)) << "get e5, newest";
  EXPECT_EQ(std::string("e6,e7,e5"), oldest(c)) << "get e5, oldest";

  c["e8"] = 500;

  EXPECT_EQ(std::string("e8,e5,e7,e6"), newest(c)) << "add e8, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8"), oldest(c)) << "add e8, oldest";

  c["e1"] = 600;

  EXPECT_EQ(std::string("e1,e8,e5,e7,e6"), newest(c)) << "re-add e1, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8,e1"), oldest(c)) << "re-add e1, oldest";
}

} // namespace tests
} // namespace base
} // namespace s3
