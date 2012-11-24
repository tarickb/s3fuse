#include <iostream>
#include <boost/function.hpp>

#include "base/static_list.h"

using std::cout;
using std::endl;

typedef s3::base::static_list<boost::function0<void> > void_fn_list;
typedef s3::base::static_list<boost::function1<int, int> > int_fn_list;

void void_fn_1()
{
  cout << "this is void fn 1" << endl;
}

void void_fn_2()
{
  cout << "this is void fn 2" << endl;
}

int int_fn_1(int a)
{
  return a + 1;
}

int int_fn_2(int a)
{
  return a * 2;
}

int int_fn_3(int a)
{
  return a * a;
}

void_fn_list::entry vf1(void_fn_1, 100);
void_fn_list::entry vf2(void_fn_2, 1); // higher priority

int_fn_list::entry if1(int_fn_1, 10);
int_fn_list::entry if2(int_fn_2, 1);
int_fn_list::entry if3(int_fn_3, 5);

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
