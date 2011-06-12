AC_PREREQ([2.67])
AC_INIT([FUSE Driver for Amazon S3], [0.1], [], [s3fuse])
AM_INIT_AUTOMAKE([no-define foreign])
AC_CONFIG_SRCDIR([src/logger.h])

AC_PROG_CXX
AC_PROG_CC

AC_CHECK_LIB([boost_thread], [main])
AC_CHECK_LIB([curl], [Curl_perform])
AC_CHECK_LIB([fuse], [fuse_opt_parse])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([pugixml], [_ZN4pugi8xml_node11append_copyERKS0_])

AC_CHECK_HEADERS([inttypes.h stdint.h stdlib.h string.h sys/time.h syslog.h])

AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_ST_BLOCKS
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftruncate gettimeofday memset strchr strerror strstr strtol])

AC_CONFIG_FILES([Makefile dist/Makefile src/Makefile])
AC_OUTPUT
