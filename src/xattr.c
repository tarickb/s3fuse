/*
 * xattr.c
 * -------------------------------------------------------------------------
 * Simple utility to list/get/set/remove extended attributes on a filesystem
 * object.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h>

const int MAX_VALUE = 1024;

#ifdef __APPLE__
  #define wrap_listxattr(path, value, size)               listxattr(path, value, size, 0)
  #define wrap_getxattr(path, name, value, size)          getxattr(path, name, value, size, 0, 0)
  #define wrap_setxattr(path, name, value, size, options) setxattr(path, name, value, size, 0, options)
  #define wrap_removexattr(path, name)                    removexattr(path, name, 0)
#else
  #define wrap_listxattr   listxattr
  #define wrap_getxattr    getxattr
  #define wrap_setxattr    setxattr
  #define wrap_removexattr removexattr
#endif

int list_attributes(const char *path)
{
  char *buffer = NULL, *b = NULL;
  int r;

  r = wrap_listxattr(path, NULL, 0);
  
  if (r > 0) {
    b = buffer = (char *)malloc(r);

    r = wrap_listxattr(path, buffer, r);

    if (r > 0) {
      while (r > 0) {
        int len = strlen(b);

        fprintf(stderr, "%s\n", b);

        b += len + 1;
        r -= len + 1;
      }

      return 0;
    }
  }

  if (r == -1) {
    fprintf(stderr, "failed to list attributes for [%s]: %s\n", path, strerror(errno));
    return r;
  }

  return 1;
}

int get_attribute(const char *path, const char *name)
{
  char buffer[MAX_VALUE];
  int r;

  r = wrap_getxattr(path, name, buffer, MAX_VALUE);

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

  r = wrap_setxattr(path, name, value, strlen(value) + 1, 0);

  if (r == -1) {
    fprintf(stderr, "failed to set attribute [%s] for [%s]: %s\n", name, path, strerror(errno));
    return r;
  }

  printf("%s: %s\n", name, value);
  return 0;
}

int remove_attribute(const char *path, const char *name)
{
  int r;

  r = wrap_removexattr(path, name);

  if (r == -1) {
    fprintf(stderr, "failed to remove attribute [%s] for [%s]: %s\n", name, path, strerror(errno));
    return r;
  }

  printf("removed %s\n", name);
  return 0;
}

int main(int argc, char **argv)
{
  int offset = 0, remove = 0;

  if (argc > 1 && strcmp(argv[1], "--remove") == 0) {
    remove = 1;
    offset++;
    argc--;
  }  

  if (argc == 2 && !remove)
    return list_attributes(argv[offset + 1]);

  if (argc == 3 && remove)
    return remove_attribute(argv[offset + 1], argv[offset + 2]);

  if (argc == 3)
    return get_attribute(argv[offset + 1], argv[offset + 2]);

  if (argc == 4 && !remove)
    return set_attribute(argv[1], argv[2], argv[3]);

  fprintf(stderr, "usage: %s [--remove] <path> [attribute-name] [attribute-value]\n", argv[0]);
  return 1;
}
