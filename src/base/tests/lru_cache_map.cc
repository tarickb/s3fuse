#include <gtest/gtest.h>
#include <functional>
#include <string>

#include "base/lru_cache_map.h"

namespace s3 {
namespace base {
namespace tests {

namespace {

bool RemoveIfOver100(const int &i) { return (i > 100); }

void AppendToString(const std::string &key, const int &i, std::string *str) {
  *str += (str->empty() ? "" : ",");
  *str += key;
}

template <class T>
std::string Oldest(const T &t) {
  std::string s;
  t.ForEachOldest(std::bind(AppendToString, std::placeholders::_1,
                            std::placeholders::_2, &s));
  return s;
}

template <class T>
std::string Newest(const T &t) {
  std::string s;
  t.ForEachNewest(std::bind(AppendToString, std::placeholders::_1,
                            std::placeholders::_2, &s));
  return s;
}

}  // namespace

TEST(LruCacheMap, NoRemoveCondition) {
  LruCacheMap<std::string, int> c(5);

  c["e1"] = 1;
  c["e2"] = 2;
  c["e3"] = 101;
  c["e4"] = 102;

  EXPECT_EQ(static_cast<size_t>(4), c.size());

  EXPECT_EQ(std::string("e4,e3,e2,e1"), Newest(c)) << "init, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4"), Oldest(c)) << "init, oldest";

  c["e5"] = 200;

  EXPECT_EQ(std::string("e5,e4,e3,e2,e1"), Newest(c)) << "add e5, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4,e5"), Oldest(c)) << "add e5, oldest";

  c["e6"] = 300;

  EXPECT_EQ(std::string("e6,e5,e4,e3,e2"), Newest(c)) << "add e6, newest";
  EXPECT_EQ(std::string("e2,e3,e4,e5,e6"), Oldest(c)) << "add e6, oldest";

  EXPECT_EQ(2, c["e2"]);

  EXPECT_EQ(std::string("e2,e6,e5,e4,e3"), Newest(c)) << "get e2, newest";
  EXPECT_EQ(std::string("e3,e4,e5,e6,e2"), Oldest(c)) << "get e2, oldest";

  c["e7"] = 400;

  EXPECT_EQ(std::string("e7,e2,e6,e5,e4"), Newest(c)) << "add e7, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e2,e7"), Oldest(c)) << "add e7, oldest";

  c.Erase("e1");

  EXPECT_EQ(std::string("e7,e2,e6,e5,e4"), Newest(c)) << "erase e1, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e2,e7"), Oldest(c)) << "erase e1, oldest";

  c.Erase("e2");

  EXPECT_EQ(std::string("e7,e6,e5,e4"), Newest(c)) << "erase e2, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e7"), Oldest(c)) << "erase e2, oldest";

  EXPECT_EQ(400, c["e7"]);

  EXPECT_EQ(std::string("e7,e6,e5,e4"), Newest(c)) << "get e7, newest";
  EXPECT_EQ(std::string("e4,e5,e6,e7"), Oldest(c)) << "get e7, oldest";

  EXPECT_EQ(200, c["e5"]);

  EXPECT_EQ(std::string("e5,e7,e6,e4"), Newest(c)) << "get e5, newest";
  EXPECT_EQ(std::string("e4,e6,e7,e5"), Oldest(c)) << "get e5, oldest";

  c["e8"] = 500;

  EXPECT_EQ(std::string("e8,e5,e7,e6,e4"), Newest(c)) << "add e8, newest";
  EXPECT_EQ(std::string("e4,e6,e7,e5,e8"), Oldest(c)) << "add e8, oldest";

  c["e1"] = 600;

  EXPECT_EQ(std::string("e1,e8,e5,e7,e6"), Newest(c)) << "re-add e1, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8,e1"), Oldest(c)) << "re-add e1, oldest";
}

TEST(LruCacheMap, RemoveIfOver100) {
  LruCacheMap<std::string, int, RemoveIfOver100> c(5);

  c["e1"] = 1;
  c["e2"] = 2;
  c["e3"] = 101;
  c["e4"] = 102;

  EXPECT_EQ(static_cast<size_t>(4), c.size());

  EXPECT_EQ(std::string("e4,e3,e2,e1"), Newest(c)) << "init, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4"), Oldest(c)) << "init, oldest";

  c["e5"] = 200;

  EXPECT_EQ(std::string("e5,e4,e3,e2,e1"), Newest(c)) << "add e5, newest";
  EXPECT_EQ(std::string("e1,e2,e3,e4,e5"), Oldest(c)) << "add e5, oldest";

  c["e6"] = 300;

  EXPECT_EQ(std::string("e6,e5,e4,e2,e1"), Newest(c)) << "add e6, newest";
  EXPECT_EQ(std::string("e1,e2,e4,e5,e6"), Oldest(c)) << "add e6, oldest";

  EXPECT_EQ(2, c["e2"]);

  EXPECT_EQ(std::string("e2,e6,e5,e4,e1"), Newest(c)) << "get e2, newest";
  EXPECT_EQ(std::string("e1,e4,e5,e6,e2"), Oldest(c)) << "get e2, oldest";

  c["e7"] = 400;

  EXPECT_EQ(std::string("e7,e2,e6,e5,e1"), Newest(c)) << "add e7, newest";
  EXPECT_EQ(std::string("e1,e5,e6,e2,e7"), Oldest(c)) << "add e7, oldest";

  c.Erase("e1");

  EXPECT_EQ(std::string("e7,e2,e6,e5"), Newest(c)) << "erase e1, newest";
  EXPECT_EQ(std::string("e5,e6,e2,e7"), Oldest(c)) << "erase e1, oldest";

  c.Erase("e2");

  EXPECT_EQ(std::string("e7,e6,e5"), Newest(c)) << "erase e2, newest";
  EXPECT_EQ(std::string("e5,e6,e7"), Oldest(c)) << "erase e2, oldest";

  EXPECT_EQ(400, c["e7"]);

  EXPECT_EQ(std::string("e7,e6,e5"), Newest(c)) << "get e7, newest";
  EXPECT_EQ(std::string("e5,e6,e7"), Oldest(c)) << "get e7, oldest";

  EXPECT_EQ(200, c["e5"]);

  EXPECT_EQ(std::string("e5,e7,e6"), Newest(c)) << "get e5, newest";
  EXPECT_EQ(std::string("e6,e7,e5"), Oldest(c)) << "get e5, oldest";

  c["e8"] = 500;

  EXPECT_EQ(std::string("e8,e5,e7,e6"), Newest(c)) << "add e8, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8"), Oldest(c)) << "add e8, oldest";

  c["e1"] = 600;

  EXPECT_EQ(std::string("e1,e8,e5,e7,e6"), Newest(c)) << "re-add e1, newest";
  EXPECT_EQ(std::string("e6,e7,e5,e8,e1"), Oldest(c)) << "re-add e1, oldest";
}

}  // namespace tests
}  // namespace base
}  // namespace s3
