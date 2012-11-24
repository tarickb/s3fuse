#include <iostream>

#include "base/tests/static_list_multi.h"

using std::cout;
using std::endl;

using s3::base::tests::void_fn_list;
using s3::base::tests::int_fn_list;

void void_fn_2()
{
  cout << "this is void fn 2" << endl;
}

int int_fn_2(int a)
{
  return a * a;
}

void_fn_list::entry vf2(void_fn_2, 100);
int_fn_list::entry if2(int_fn_2, 10);
