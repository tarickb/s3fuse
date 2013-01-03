#include <gtest/gtest.h>

#include <boost/function.hpp>

#include "base/static_list.h"

using std::string;

typedef s3::base::static_list<boost::function1<string, const char *> > str_fn_list;
typedef s3::base::static_list<boost::function1<string, double> > double_fn_list;

string str_fn_1(const char *a)
{
  return "str fn 1";
}

string str_fn_2(const char *a)
{
  return "str fn 2";
}

string double_fn_1(double a)
{
  return "double fn 1";
}

string double_fn_2(double a)
{
  return "double fn 2";
}

string double_fn_3(double a)
{
  return "double fn 3";
}

str_fn_list::entry sf1(str_fn_1, 100);
str_fn_list::entry sf2(str_fn_2, 1); // higher priority

double_fn_list::entry df1(double_fn_1, 10);
double_fn_list::entry df2(double_fn_2, 1);
double_fn_list::entry df3(double_fn_3, 5);

TEST(static_list, single_sequence)
{
  string s;

  for (str_fn_list::const_iterator itor = str_fn_list::begin(); itor != str_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(NULL);

  EXPECT_EQ(string("str fn 2,str fn 1"), s);

  s.clear();

  for (double_fn_list::const_iterator itor = double_fn_list::begin(); itor != double_fn_list::end(); ++itor)
    s += (s.empty() ? "" : ",") + itor->second(0);

  EXPECT_EQ(string("double fn 2,double fn 3,double fn 1"), s);
}
