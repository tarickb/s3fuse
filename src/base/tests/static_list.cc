#include <functional>
#include <gtest/gtest.h>

#include "base/static_list.h"

namespace s3 {
namespace base {
namespace tests {

using str_fn_list = static_list<std::function<std::string(const char *)>>;
using double_fn_list = static_list<std::function<std::string(double)>>;

std::string str_fn_1(const char *a) { return "str fn 1"; }

std::string str_fn_2(const char *a) { return "str fn 2"; }

std::string double_fn_1(double a) { return "double fn 1"; }

std::string double_fn_2(double a) { return "double fn 2"; }

std::string double_fn_3(double a) { return "double fn 3"; }

str_fn_list::entry sf1(str_fn_1, 100);
str_fn_list::entry sf2(str_fn_2, 1); // higher priority

double_fn_list::entry df1(double_fn_1, 10);
double_fn_list::entry df2(double_fn_2, 1);
double_fn_list::entry df3(double_fn_3, 5);

TEST(static_list, single_sequence) {
  std::string s;

  for (str_fn_list::const_iterator itor = str_fn_list::begin();
       itor != str_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(NULL);

  EXPECT_EQ(std::string("str fn 2,str fn 1"), s);

  s.clear();

  for (double_fn_list::const_iterator itor = double_fn_list::begin();
       itor != double_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(0);

  EXPECT_EQ(std::string("double fn 2,double fn 3,double fn 1"), s);
}

} // namespace tests
} // namespace base
} // namespace s3
