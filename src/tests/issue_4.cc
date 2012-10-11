#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#define STR_(x) #x
#define STR(x) STR_(x)

#define T(x, e) do { int r = (x); if (r != (e)) { perror(#x " on line " STR(__LINE__) " failed with error"); return 1; } } while (0)
#define TZ(x) T(x, 0)

namespace
{
  const char TEST_STRING[] = "this is a test!\n";
  const int TEST_STRING_LEN = sizeof(TEST_STRING);
}

int main(int argc, char **argv)
{
  struct stat st;
  const char *file = NULL;
  int create_fd = -1, open_fd = -1;

  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <test-file-name>" << endl;
    return 1;
  }

  file = argv[1];


  /*
   * getattr
   * unlink
   * getattr
   * create
   * getattr
   * open
   * chmod
   * getattr
   * removexattr
   * setxattr
   * getattr
   * chmod
   * getattr
   * flush
   * release
   * getattr
   * release
   */

  TZ(stat(file, &st));
  TZ(unlink(file));
  T(stat(file, &st), -1);

  create_fd = open(file, O_CREAT);

  if (create_fd == -1) {
    perror("create failed with error");
    return 1;
  }

  TZ(fstat(create_fd, &st));

  open_fd = open(file, O_RDWR);

  if (open_fd == -1) {
    perror("open failed with error");
    return 1;
  }

  TZ(fchmod(open_fd, 0755));
  TZ(fstat(open_fd, &st));

  if (write(open_fd, TEST_STRING, TEST_STRING_LEN) != TEST_STRING_LEN) {
    perror("write failed with error");
    return 1;
  }

  TZ(fsync(open_fd));
  TZ(close(open_fd));
  TZ(fstat(create_fd, &st));
  TZ(close(create_fd));

  cout << "succeeded." << endl;
  return 0;
}
