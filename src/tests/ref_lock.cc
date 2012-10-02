#include <iostream>
#include <map>
#include <vector>
#include <boost/smart_ptr.hpp>

#include "ref_lock.h"

using namespace boost;
using namespace std;

using namespace s3;

class test
{
public:
  typedef shared_ptr<test> ptr;
  typedef ref_lock<test> lock;

  inline test(int id)
    : _id(id),
      _ref_count(0)
  {
    cout << "test [" << _id << "]: ctor" << endl;
  }

  inline ~test()
  {
    cout << "test [" << _id << "]: dtor with ref count: " << _ref_count << endl;
  }

  inline void print_id()
  {
    cout << "test [" << _id << "]: hello!" << endl;
  }

private:
  friend class ref_lock<test>;

  inline void add_ref()
  {
    _ref_count++;

    cout << "test [" << _id << "]: add ref, new count: " << _ref_count << endl;
  }

  inline void release_ref()
  {
    _ref_count--;

    cout << "test [" << _id << "]: release ref, new count: " << _ref_count << endl;
  }

  int _id;
  int _ref_count;
};

int main(int argc, char **argv)
{
  vector<test::lock> test_vec;
  map<int, test::lock> test_map;

  cout << "adding to test_vec" << endl;

  for (int i = 0; i < 5; i++)
    test_vec.push_back(test::lock(test::ptr(new test(i))));

  cout << "testing at position 2" << endl;

  test_vec[2]->print_id();
  (*test_vec[2]).print_id();

  cout << "adding to test_map" << endl;

  for (int i = 100; i < 105; i++)
    test_map[i] = test::lock(test::ptr(new test(i)));

  cout << "testing at key 105" << endl;

  if (test_map[103].is_valid()) {
    test_map[103]->print_id();
    (*test_map[103]).print_id();
  }

  cout << "assignment test" << endl;

  test::lock l;

  l = test_map[100];
  l->print_id();
  l = test_map[102];
  l->print_id();

  cout << "done" << endl;

  return 0;
}
