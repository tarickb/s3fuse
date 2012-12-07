#include <iostream>
#include <vector>
#include <boost/bind.hpp>

#include "fs/callback_xattr.h"

using boost::bind;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using s3::fs::callback_xattr;
using s3::fs::xattr;

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
    &buf[0] << endl;
}

int get(const string &key, string *val)
{
  cout << "get: key: " << key << endl;
  *val = s_val;

  return 0;
}

int set(const string &key, const string &val)
{
  cout << "set: key: " << key << ", value: " << val << endl;
  s_val = val;

  return 0;
}

int main(int argc, char **argv)
{
  const char *TEST = "test";
  xattr::ptr p;

  p = callback_xattr::create(
    "cb_test_key", 
    bind(get, _1, _2),
    bind(set, _1, _2));

  cout << "test 1" << endl;
  test(p);

  cout << "setting..." << endl;
  p->set_value(TEST, strlen(TEST));

  cout << "test 2" << endl;
  test(p);

  return 0;
}
