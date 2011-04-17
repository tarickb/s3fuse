#ifndef S3_DEBUG_HH
#define S3_DEBUG_HH

#include <pthread.h>
#include <stdio.h>

#define S3_DEBUG(fn, str, ...) fprintf(stderr, fn " [%lx]: " str, pthread_self(), __VA_ARGS__)

#endif
