#include <gtest/gtest.h>

#include "base/tests/static_list_multi.h"

using std::string;

using s3::base::tests::int_fn_list;
using s3::base::tests::void_fn_list;

TEST(static_list, multi_sequence)
{
  string s;

  for (void_fn_list::const_iterator itor = void_fn_list::begin(); itor != void_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second();

  EXPECT_EQ(string("void fn 1,void fn 2"), s);

  s.clear();

  for (int_fn_list::const_iterator itor = int_fn_list::begin(); itor != int_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(0);

  EXPECT_EQ(string("int fn 1,int fn 2"), s);
}
