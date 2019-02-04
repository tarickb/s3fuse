#include <iostream>

#include "fs/static_xattr.h"

void test(const s3::fs::xattr::ptr &p)
{
  std::string hk, hv;

  p->to_header(&hk, &hv);

  std::cout << 
    p->get_key() << ": " << 
    (p->is_writable() ? "(writable) " : "") << 
    (p->is_serializable() ? "(serializable) " : "") << 
    (p->is_visible() ? "(visible) " : "") <<
    (p->is_removable() ? "(removable) " : "") <<
    hk << ": " <<
    hv << std::endl;
}

int main(int argc, char **argv)
{
  const int LEN = 1024;
  char buf[LEN], buf_out[LEN];
  std::string hk, hv;
  s3::fs::xattr::ptr p;

  srand(time(NULL));

  for (int i = 0; i < LEN; i++)
    buf[i] = rand();

  test(s3::fs::static_xattr::from_string("x1", "abcdef", s3::fs::xattr::XM_VISIBLE));
  test(s3::fs::static_xattr::create("x2", s3::fs::xattr::XM_VISIBLE));
  test(s3::fs::static_xattr::create("x3", s3::fs::xattr::XM_WRITABLE));
  test(s3::fs::static_xattr::from_string("x4", "blah",
        s3::fs::xattr::XM_WRITABLE | s3::fs::xattr::XM_SERIALIZABLE));
  test(s3::fs::static_xattr::from_string("AN_UPPERCASE_KEY", "value", s3::fs::xattr::XM_SERIALIZABLE));

  p = s3::fs::static_xattr::create("should_be_AN_INVALID_KEY",
      s3::fs::xattr::XM_WRITABLE | s3::fs::xattr::XM_SERIALIZABLE);

  p->set_value(buf, LEN);
  test(p);

  p->to_header(&hk, &hv);

  p = s3::fs::static_xattr::from_header(hk, hv, s3::fs::xattr::XM_VISIBLE);
  test(p);

  std::cout << "get_value: " << p->get_value(buf_out, LEN) << std::endl;

  for (int i = 0; i < LEN; i++) {
    if (buf[i] != buf_out[i]) {
      std::cout << "values do not match at position " << i << std::endl;
      break;
    }
  }

  std::cout << "values match" << std::endl;

  return 0;
}
