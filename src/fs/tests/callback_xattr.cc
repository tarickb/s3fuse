#include <functional>
#include <iostream>
#include <vector>

#include "fs/callback_xattr.h"

using std::bind;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using s3::fs::callback_xattr;
using s3::fs::xattr;

using namespace std::placeholders;

string s_val;

void test(const xattr::ptr &p)
{
  vector<char> buf;
  int len;

  len = p->get_value(NULL, 0);

  if (len < 0) {
    cout << "error: " << len << endl;
    return;
  }

  buf.resize(len + 1);

  if (p->get_value(&buf[0], len) != len) {
    cout << "error in get_value()" << endl;
    return;
  }

  buf[len] = '\0';

  cout << 
    p->get_key() << ": " << 
    (p->is_writable() ? "(writable) " : "") << 
    (p->is_serializable() ? "(serializable) " : "") << 
    (p->is_visible() ? "(visible) " : "") <<
    (p->is_removable() ? "(removable) " : "") <<
    &buf[0] << endl;
}

int get(string *val)
{
  cout << "get" << endl;
  *val = s_val;

  return 0;
}

int set(const string &val)
{
  cout << "set: value: " << val << endl;
  s_val = val;

  return 0;
}

int main(int argc, char **argv)
{
  const string TEST = "test";
  xattr::ptr p;

  p = callback_xattr::create(
    "cb_test_key", 
    bind(get, _1),
    bind(set, _1),
    xattr::XM_VISIBLE);

  cout << "test 1" << endl;
  test(p);

  cout << "setting..." << endl;
  p->set_value(TEST.c_str(), TEST.size());

  cout << "test 2" << endl;
  test(p);

  return 0;
}
