#include <gtest/gtest.h>

#include "base/tests/static_list_multi.h"

namespace s3 {
namespace base {
namespace tests {

TEST(static_list, multi_sequence) {
  std::string s;

  for (void_fn_list::const_iterator itor = void_fn_list::begin();
       itor != void_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second();

  EXPECT_EQ(std::string("void fn 1,void fn 2"), s);

  s.clear();

  for (int_fn_list::const_iterator itor = int_fn_list::begin();
       itor != int_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(0);

  EXPECT_EQ(std::string("int fn 1,int fn 2"), s);
}

} // namespace tests
} // namespace base
} // namespace s3
