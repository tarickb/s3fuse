#include <iostream>

#include "base/tests/static_list_multi.h"

using std::cout;
using std::endl;

using s3::base::tests::void_fn_list;
using s3::base::tests::int_fn_list;

void void_fn_1()
{
  cout << "this is void fn 1" << endl;
}

int int_fn_1(int a)
{
  return a + 1;
}

void_fn_list::entry vf1(void_fn_1, 100);
int_fn_list::entry if1(int_fn_1, 10);
