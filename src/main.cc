/*
 * main.cc
 * -------------------------------------------------------------------------
 * FUSE driver for S3.  Wraps calls to s3::fs.
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

#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fuse.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "config.h"
#include "directory.h"
#include "file.h"
#include "logger.h"
#include "object_cache.h"
#include "service.h"
#include "ssl_locks.h"
#include "symlink.h"
#include "version.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

// also adjust path by skipping leading slash
#define ASSERT_VALID_PATH(str) \
  do { \
    if ((str)[0] != '/' || ((str)[1] != '\0' && (str)[strlen(str) - 1] == '/')) { \
      S3_LOG(LOG_WARNING, "ASSERT_VALID_PATH", "failed on [%s]\n", static_cast<const char *>(str)); \
      return -EINVAL; \
    } \
    \
    (str)++; \
  } while (0)

#define BEGIN_TRY \
  try {

#define END_TRY \
  } catch (const std::exception &e) { \
    S3_LOG(LOG_WARNING, "END_TRY", "caught exception: %s (at line %i)\n", e.what(), __LINE__); \
    return -ECANCELED; \
  } catch (...) { \
    S3_LOG(LOG_WARNING, "END_TRY", "caught unknown exception (at line %i)\n", __LINE__); \
    return -ECANCELED; \
  }

#define GET_OBJECT(var, path) \
  object::ptr var = object_cache::get(path); \
  \
  if (!var) \
    return -ENOENT;

#define GET_OBJECT_AS(type, mode, var, path) \
  type::ptr var = static_pointer_cast<type>(object_cache::get(path)); \
  \
  if (!var) \
    return -ENOENT; \
  \
  if (var->get_type() != (mode)) { \
    S3_LOG( \
      LOG_WARNING, \
      "GET_OBJECT_AS", \
      "could not get [%s] as type [%s] (requested mode %i, reported mode %i, at line %i)\n", \
      static_cast<const char *>(path), \
      #type, \
      mode, \
      var->get_type(), \
      __LINE__); \
    \
    return -EINVAL; \
  }

namespace
{
  struct options
  {
    const char *arg0;
    string config;
    string mountpoint;
    int verbosity;
    bool daemon_timeout_set;

    #ifdef __APPLE__
      bool noappledouble_set;
    #endif
  };

  void dir_filler(fuse_fill_dir_t filler, void *buf, const std::string &path)
  {
    filler(buf, path.c_str(), NULL, 0);
  }

  int s_mountpoint_mode;
}

int s3fuse_chmod(const char *path, mode_t mode)
{
  S3_LOG(LOG_DEBUG, "chmod", "path: %s, mode: %i\n", path, mode);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    obj->set_mode(mode);

    return obj->commit();
  END_TRY;
}

int s3fuse_chown(const char *path, uid_t uid, gid_t gid)
{
  S3_LOG(LOG_DEBUG, "chown", "path: %s, user: %i, group: %i\n", path, uid, gid);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    if (uid != static_cast<uid_t>(-1))
      obj->set_uid(uid);

    if (gid != static_cast<gid_t>(-1))
      obj->set_gid(gid);

    return obj->commit();
  END_TRY;
}

int s3fuse_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  int r;
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "create", "path: %s, mode: %#o\n", path, mode);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    file::ptr f;

    if (object_cache::get(path)) {
      S3_LOG(LOG_WARNING, "create", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    directory::invalidate_parent(path);

    f.reset(new file(path));

    f->set_mode(mode);
    f->set_uid(ctx->uid);
    f->set_gid(ctx->gid);

    r = f->commit();

    if (r)
      return r;

    return file::open(static_cast<string>(path), OPEN_DEFAULT, &file_info->fh);
  END_TRY;
}

int s3fuse_flush(const char *path, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "flush", "path: %s\n", f->get_path().c_str());

  BEGIN_TRY;
    return f->flush();
  END_TRY;
}

int s3fuse_ftruncate(const char *path, off_t offset, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "ftruncate", "path: %s, offset: %ji\n", f->get_path().c_str(), static_cast<intmax_t>(offset));

  BEGIN_TRY;
    return f->truncate(offset);
  END_TRY;
}

int s3fuse_getattr(const char *path, struct stat *s)
{
  ASSERT_VALID_PATH(path);

  memset(s, 0, sizeof(*s));

  if (path[0] == '\0') { // root path
    s->st_uid = geteuid();
    s->st_gid = getegid();
    s->st_mode = s_mountpoint_mode;
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    obj->copy_stat(s);

    return 0;
  END_TRY;
}

#ifdef __APPLE__
int s3fuse_getxattr(const char *path, const char *name, char *buffer, size_t max_size, uint32_t position)
#else
int s3fuse_getxattr(const char *path, const char *name, char *buffer, size_t max_size)
#endif
{
  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    return obj->get_metadata(name, buffer, max_size);
  END_TRY;
}

int s3fuse_listxattr(const char *path, char *buffer, size_t size)
{
  typedef vector<string> str_vec;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    str_vec attrs;
    size_t required_size = 0;

    obj->get_metadata_keys(&attrs);

    for (str_vec::const_iterator itor = attrs.begin(); itor != attrs.end(); ++itor)
      required_size += itor->size() + 1;

    if (buffer == NULL || size == 0)
      return required_size;

    if (required_size > size)
      return -ERANGE;

    for (str_vec::const_iterator itor = attrs.begin(); itor != attrs.end(); ++itor) {
      strcpy(buffer, itor->c_str());
      buffer += itor->size() + 1;
    }

    return required_size;
  END_TRY;
}

int s3fuse_mkdir(const char *path, mode_t mode)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "mkdir", "path: %s, mode: %#o\n", path, mode);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    directory::ptr dir;

    if (object_cache::get(path)) {
      S3_LOG(LOG_WARNING, "mkdir", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    directory::invalidate_parent(path);

    dir.reset(new directory(path));

    dir->set_mode(mode);
    dir->set_uid(ctx->uid);
    dir->set_gid(ctx->gid);

    return dir->commit();
  END_TRY;
}

int s3fuse_open(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "open", "path: %s\n", path);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    return file::open(
      static_cast<string>(path), 
      (file_info->flags & O_TRUNC) ? OPEN_TRUNCATE_TO_ZERO : OPEN_DEFAULT, 
      &file_info->fh);
  END_TRY;
}

int s3fuse_read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  BEGIN_TRY;
    return file::from_handle(file_info->fh)->read(buffer, size, offset);
  END_TRY;
}

int s3fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "readdir", "path: %s\n", path);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT_AS(directory, S_IFDIR, dir, path);

    return dir->read(bind(&dir_filler, filler, buf, _1));
  END_TRY;
}

int s3fuse_readlink(const char *path, char *buffer, size_t max_size)
{
  S3_LOG(LOG_DEBUG, "readlink", "path: %s, max_size: %zu\n", path, max_size);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT_AS(s3::symlink, S_IFLNK, link, path);

    string target;
    int r = link->read(&target);

    if (r)
      return r;

    // leave room for the terminating null
    max_size--;

    if (target.size() < max_size)
      max_size = target.size();

    memcpy(buffer, target.c_str(), max_size);
    buffer[max_size] = '\0';

    return 0;
  END_TRY;
}

int s3fuse_release(const char *path, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "release", "path: %s\n", f->get_path().c_str());

  BEGIN_TRY;
    return f->release();
  END_TRY;
}

int s3fuse_removexattr(const char *path, const char *name)
{
  S3_LOG(LOG_DEBUG, "removexattr", "path: %s, name: %s\n", path, name);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    int r = obj->remove_metadata(name);

    return r ? r : obj->commit();
  END_TRY;
}

int s3fuse_rename(const char *from, const char *to)
{
  S3_LOG(LOG_DEBUG, "rename", "from: %s, to: %s\n", from, to);

  ASSERT_VALID_PATH(from);
  ASSERT_VALID_PATH(to);

  BEGIN_TRY;
    GET_OBJECT(from_obj, from);

    // not using GET_OBJECT() here because we don't want to fail if "to"
    // doesn't exist
    object::ptr to_obj = object_cache::get(to);

    directory::invalidate_parent(from);
    directory::invalidate_parent(to);

    if (to_obj) {
      int r;

      if (to_obj->get_type() == S_IFDIR) {
        if (from_obj->get_type() != S_IFDIR)
          return -EISDIR;

        if (!static_pointer_cast<directory>(to_obj)->is_empty())
          return -ENOTEMPTY;

      } else if (from_obj->get_type() == S_IFDIR) {
        return -ENOTDIR;
      }

      r = to_obj->remove();

      if (r)
        return r;
    }

    return from_obj->rename(to);
  END_TRY;
}

#ifdef __APPLE__
int s3fuse_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
#else
int s3fuse_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
#endif
{
  S3_LOG(LOG_DEBUG, "setxattr", "path: [%s], name: [%s], size: %i\n", path, name, size);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    int r = obj->set_metadata(name, value, size, flags);

    return r ? r : obj->commit();
  END_TRY;
}

int s3fuse_symlink(const char *target, const char *path)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "symlink", "path: %s, target: %s\n", path, target);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    symlink::ptr link;

    if (object_cache::get(path)) {
      S3_LOG(LOG_WARNING, "symlink", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    directory::invalidate_parent(path);

    link.reset(new s3::symlink(path));

    link->set_uid(ctx->uid);
    link->set_gid(ctx->gid);

    link->set_target(target);

    return link->commit();
  END_TRY;
}

int s3fuse_unlink(const char *path)
{
  S3_LOG(LOG_DEBUG, "unlink", "path: %s\n", path);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    directory::invalidate_parent(path);

    return obj->remove();
  END_TRY;
}

int s3fuse_utimens(const char *path, const timespec times[2])
{
  S3_LOG(LOG_DEBUG, "utimens", "path: %s, time: %li\n", path, times[1].tv_sec);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    obj->set_mtime(times[1].tv_sec);

    return obj->commit();
  END_TRY;
}

int s3fuse_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  BEGIN_TRY;
    return file::from_handle(file_info->fh)->write(buffer, size, offset);
  END_TRY;
}

int print_version()
{
  printf("%s, %s, version %s\n", s3::APP_FULL_NAME, s3::APP_NAME, s3::APP_VERSION);

  return 0;
}

int print_usage(const char *arg0)
{
  fprintf(stderr, 
    "Usage: %s [options] <mountpoint>\n"
    "\n"
    "Options:\n"
    "  -f                   stay in the foreground (i.e., do not daemonize)\n"
    "  -h, --help           print this help message and exit\n"
    "  -o OPT...            pass OPT (comma-separated) to FUSE, such as:\n"
    "     allow_other         allow other users to access the mounted file system\n"
    "     allow_root          allow root to access the mounted file system\n"
    "     config=<file>       use <file> rather than the default configuration file\n"
    "     daemon_timeout=<n>  set fuse timeout to <n> seconds\n"
    "     noappledouble       disable testing for/creating .DS_Store files on Mac OS\n"
    "  -v, --verbose        enable logging to stderr (can be repeated for more verbosity)\n"
    "  -V, --version        print version and exit\n",
    arg0);

  return 0;
}

int process_argument(void *data, const char *arg, int key, struct fuse_args *out_args)
{
  options *opt = static_cast<options *>(data);

  if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
    print_version();
    exit(0);
  }

  if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
    print_usage(opt->arg0);
    exit(1);
  }

  if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
    opt->verbosity++;
    return 0;
  }

  if (strstr(arg, "config=") == arg) {
    opt->config = arg + 7;
    return 0;
  }

  if (strstr(arg, "daemon_timeout=") == arg) {
    opt->daemon_timeout_set = true;
    return 1; // continue processing
  }

  #ifdef __APPLE__
    if (strstr(arg, "noappledouble") == arg) {
      opt->noappledouble_set = true;
      return 1; // continue processing
    }
  #endif

  if (key == FUSE_OPT_KEY_NONOPT)
    opt->mountpoint = arg; // assume that the mountpoint is the only non-option

  return 1;
}

int pre_init(const options &opts)
{
  try {
    struct stat mp_stat;

    logger::init(opts.verbosity);

    if (stat(opts.mountpoint.c_str(), &mp_stat))
      throw runtime_error("failed to stat mount point.");

    s_mountpoint_mode = S_IFDIR | mp_stat.st_mode;

    return config::init(opts.config);

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "pre_init", "caught exception while initializing: %s\n", e.what());

  } catch (...) {
    S3_LOG(LOG_ERR, "pre_init", "caught unknown exception while initializing.\n");
  }

  return -EIO;
}

void * init(fuse_conn_info *info)
{
  try {
    ssl_locks::init();
    thread_pool::init();

    service::init(config::get_service());
    xml::init(service::get_xml_namespace());

    object_cache::init();

    if (info->capable & FUSE_CAP_ATOMIC_O_TRUNC) {
      info->want |= FUSE_CAP_ATOMIC_O_TRUNC;
      S3_LOG(LOG_DEBUG, "init", "enabling FUSE_CAP_ATOMIC_O_TRUNC\n");
    } else {
      S3_LOG(LOG_WARNING, "init", "FUSE_CAP_ATOMIC_O_TRUNC not supported, will revert to truncate-then-open\n");
    }

    return NULL;

  } catch (const std::exception &e) {
    S3_LOG(LOG_ERR, "init", "caught exception while initializing: %s\n", e.what());

  } catch (...) {
    S3_LOG(LOG_ERR, "init", "caught unknown exception while initializing.\n");
  }

  fuse_exit(fuse_get_context()->fuse);
  return NULL;
}

void build_ops(fuse_operations *ops)
{
  memset(ops, 0, sizeof(*ops));

  // TODO: add truncate()

  ops->flag_nullpath_ok = 1;

  ops->chmod = s3fuse_chmod;
  ops->chown = s3fuse_chown;
  ops->create = s3fuse_create;
  ops->getattr = s3fuse_getattr;
  ops->getxattr = s3fuse_getxattr;
  ops->flush = s3fuse_flush;
  ops->ftruncate = s3fuse_ftruncate;
  ops->init = init;
  ops->listxattr = s3fuse_listxattr;
  ops->mkdir = s3fuse_mkdir;
  ops->open = s3fuse_open;
  ops->read = s3fuse_read;
  ops->readdir = s3fuse_readdir;
  ops->readlink = s3fuse_readlink;
  ops->release = s3fuse_release;
  ops->removexattr = s3fuse_removexattr;
  ops->rename = s3fuse_rename;
  ops->rmdir = s3fuse_unlink;
  ops->setxattr = s3fuse_setxattr;
  ops->symlink = s3fuse_symlink;
  ops->unlink = s3fuse_unlink;
  ops->utimens = s3fuse_utimens;
  ops->write = s3fuse_write;
}

int main(int argc, char **argv)
{
  int r;
  fuse_operations ops;
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  options opts;

  opts.verbosity = LOG_WARNING;
  opts.arg0 = argv[0];
  opts.daemon_timeout_set = false;

  #ifdef __APPLE__
    opts.noappledouble_set = false;
  #endif

  fuse_opt_parse(&args, &opts, NULL, process_argument);

  if (opts.mountpoint.empty()) {
    print_usage(opts.arg0);
    exit(1);
  }

  if (!opts.daemon_timeout_set)
    fprintf(stderr, "Set \"-o daemon_timeout=3600\" or something equally large if transferring large files, otherwise FUSE will time out.\n");

  #ifdef __APPLE__
    if (!opts.noappledouble_set)
      fprintf(stderr, "You are *strongly* advised to pass \"-o noappledouble\" to disable the creation/checking/etc. of .DS_Store files.\n");
  #endif

  build_ops(&ops);

  if ((r = pre_init(opts)))
    return r;

  r = fuse_main(args.argc, args.argv, &ops, NULL);

  fuse_opt_free_args(&args);

  thread_pool::terminate();
  object_cache::print_summary();
  ssl_locks::release();

  return r;
}
