#include <stdio.h>

#define S3_DEBUG(fn, str, ...) fprintf(stderr, fn ": " str, __VA_ARGS__)
