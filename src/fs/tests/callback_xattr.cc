#include <functional>
#include <iostream>
#include <vector>

#include "fs/callback_xattr.h"

std::string s_val;

void test(const s3::fs::xattr::ptr &p)
{
  std::vector<char> buf;
  int len;

  len = p->get_value(NULL, 0);

  if (len < 0) {
    std::cout << "error: " << len << std::endl;
    return;
  }

  buf.resize(len + 1);

  if (p->get_value(&buf[0], len) != len) {
    std::cout << "error in get_value()" << std::endl;
    return;
  }

  buf[len] = '\0';

  std::cout << 
    p->get_key() << ": " << 
    (p->is_writable() ? "(writable) " : "") << 
    (p->is_serializable() ? "(serializable) " : "") << 
    (p->is_visible() ? "(visible) " : "") <<
    (p->is_removable() ? "(removable) " : "") <<
    &buf[0] << std::endl;
}

int get(std::string *val)
{
  std::cout << "get" << std::endl;
  *val = s_val;

  return 0;
}

int set(const std::string &val)
{
  std::cout << "set: value: " << val << std::endl;
  s_val = val;

  return 0;
}

int main(int argc, char **argv)
{
  const std::string TEST = "test";
  s3::fs::xattr::ptr p;

  p = s3::fs::callback_xattr::create(
    "cb_test_key", 
    std::bind(get, std::placeholders::_1),
    std::bind(set, std::placeholders::_1),
    s3::fs::xattr::XM_VISIBLE);

  std::cout << "test 1" << std::endl;
  test(p);

  std::cout << "setting..." << std::endl;
  p->set_value(TEST.c_str(), TEST.size());

  std::cout << "test 2" << std::endl;
  test(p);

  return 0;
}
