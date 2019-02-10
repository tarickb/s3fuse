#include <gtest/gtest.h>

#include "base/tests/static_list_multi.h"

namespace s3 {
namespace base {
namespace tests {

TEST(StaticList, MultiSequence) {
  std::string s;

  for (auto iter = VoidFunctionList::begin(); iter != VoidFunctionList::end();
       ++iter)
    s += (s.empty() ? "" : ",") + iter->second();

  EXPECT_EQ(std::string("void fn 1,void fn 2"), s);

  s.clear();

  for (auto iter = IntFunctionList::begin(); iter != IntFunctionList::end();
       ++iter)
    s += (s.empty() ? "" : ",") + iter->second(0);

  EXPECT_EQ(std::string("int fn 1,int fn 2"), s);
}

}  // namespace tests
}  // namespace base
}  // namespace s3
