#include <iostream>

#include "base/tests/static_list_multi.h"

using std::cout;
using std::endl;

using s3::base::tests::int_fn_list;
using s3::base::tests::void_fn_list;

int main(int argc, char **argv)
{
  const int INT_IN = 123;

  cout << "void functions:" << endl;

  for (void_fn_list::const_iterator itor = void_fn_list::begin(); itor != void_fn_list::end(); ++itor)
    itor->second();

  cout << "int functions:" << endl;

  for (int_fn_list::const_iterator itor = int_fn_list::begin(); itor != int_fn_list::end(); ++itor)
    cout << "in: " << INT_IN << ", out: " << itor->second(INT_IN) << endl;

  return 0;
}
