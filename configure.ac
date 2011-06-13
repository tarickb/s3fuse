AC_PREREQ([2.67])
AC_INIT([FUSE Driver for Amazon S3], m4_esyscmd_s([echo $VERSION]), [], [s3fuse])
AM_INIT_AUTOMAKE([no-define foreign])
AC_CONFIG_SRCDIR([src/logger.h])

AC_PROG_CXX
AC_PROG_CC

AC_ARG_ENABLE(
  [build-rpm], 
  [  --enable-build-rpm Enable RPM build support],
  [case "${enableval}" in
    yes) build_rpm=true ;;
    no)  build_rpm=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-build-rpm]) ;;
   esac],[build_rpm=false])

AM_CONDITIONAL([BUILD_RPM], [test x$build_rpm = xtrue])

AC_CHECK_LIB([boost_thread], [main])
AC_CHECK_LIB([curl], [Curl_perform])
AC_CHECK_LIB([fuse], [fuse_opt_parse])
AC_CHECK_LIB([pthread], [pthread_create])

PKG_CHECK_MODULES([DEPS], [libxml-2.0 >= 2.7.8 libxml++-2.6 >= 2.30.0])

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

AC_CONFIG_FILES([Makefile src/Makefile])
AM_COND_IF([BUILD_RPM], [AC_CONFIG_FILES([dist/Makefile])])

AC_OUTPUT
