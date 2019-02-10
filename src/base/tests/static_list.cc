#include <gtest/gtest.h>
#include <functional>

#include "base/static_list.h"

namespace s3 {
namespace base {
namespace tests {

using StringFunctionList = StaticList<std::function<std::string(const char *)>>;
using DoubleFunctionList = StaticList<std::function<std::string(double)>>;

std::string StringFunction1(const char *a) { return "str fn 1"; }

std::string StringFunction2(const char *a) { return "str fn 2"; }

std::string DoubleFunction1(double a) { return "double fn 1"; }

std::string DoubleFunction2(double a) { return "double fn 2"; }

std::string DoubleFunction3(double a) { return "double fn 3"; }

StringFunctionList::Entry sf1(StringFunction1, 100);
StringFunctionList::Entry sf2(StringFunction2, 1);  // higher priority

DoubleFunctionList::Entry df1(DoubleFunction1, 10);
DoubleFunctionList::Entry df2(DoubleFunction2, 1);
DoubleFunctionList::Entry df3(DoubleFunction3, 5);

TEST(StaticList, SingleSequence) {
  std::string s;

  for (auto iter = StringFunctionList::begin();
       iter != StringFunctionList::end(); ++iter)
    s += (s.empty() ? "" : ",") + iter->second(nullptr);

  EXPECT_EQ(std::string("str fn 2,str fn 1"), s);

  s.clear();

  for (auto iter = DoubleFunctionList::begin();
       iter != DoubleFunctionList::end(); ++iter)
    s += (s.empty() ? "" : ",") + iter->second(0);

  EXPECT_EQ(std::string("double fn 2,double fn 3,double fn 1"), s);
}

}  // namespace tests
}  // namespace base
}  // namespace s3
