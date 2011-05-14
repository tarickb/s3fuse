#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/xattr.h>

const int MAX_VALUE = 1024;

int list_attributes(const char *path)
{
  return 1;
}

int get_attribute(const char *path, const char *name)
{
  char buffer[MAX_VALUE];
  int r;

  r = getxattr(path, name, buffer, MAX_VALUE);

  if (r == -1) {
    fprintf(stderr, "failed to get attribute [%s] for [%s]: %s\n", name, path, strerror(errno));
    return r;
  }

  printf("%s: %s\n", name, buffer);
  return 0;
}

int set_attribute(const char *path, const char *name, const char *value)
{
  int r;

  r = setxattr(path, name, value, strlen(value) + 1, 0);

  if (r == -1) {
    fprintf(stderr, "failed to set attribute [%s] for [%s]: %s\n", name, path, strerror(errno));
    return r;
  }

  printf("%s: %s\n", name, value);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc == 2)
    return list_attributes(argv[1]);

  if (argc == 3)
    return get_attribute(argv[1], argv[2]);

  if (argc == 4)
    return set_attribute(argv[1], argv[2], argv[3]);

  fprintf(stderr, "usage: %s <path> [attribute-name] [attribute-value]\n", argv[0]);
  return 1;
}
