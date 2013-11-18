/*
 * operations.cc
 * -------------------------------------------------------------------------
 * Filesystem operation implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#include <limits>
#include <boost/detail/atomic_count.hpp>

#include "operations.h"
#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "base/timer.h"
#include "fs/cache.h"
#include "fs/directory.h"
#include "fs/encrypted_file.h"
#include "fs/file.h"
#include "fs/special.h"
#include "fs/symlink.h"

using boost::static_pointer_cast;
using boost::detail::atomic_count;
using std::numeric_limits;
using std::ostream;
using std::runtime_error;
using std::string;
using std::vector;

using s3::operations;
using s3::base::config;
using s3::base::statistics;
using s3::base::timer;
using s3::fs::cache;
using s3::fs::directory;
using s3::fs::encrypted_file;
using s3::fs::file;
using s3::fs::object;
using s3::fs::special;
using s3::fs::symlink;

namespace
{
  atomic_count s_reopen_attempts(0), s_reopen_rescues(0), s_reopen_fails(0);
  atomic_count s_rename_attempts(0), s_rename_fails(0);
  atomic_count s_create(0), s_mkdir(0), s_mknod(0), s_open(0), s_rename(0), s_symlink(0), s_truncate(0), s_unlink(0);
  atomic_count s_getattr(0), s_readdir(0), s_readlink(0);

  void dir_filler(fuse_fill_dir_t filler, void *buf, const std::string &path)
  {
    filler(buf, path.c_str(), NULL, 0);
  }

  inline string get_parent(const string &path)
  {
    size_t last_slash = path.rfind('/');

    return (last_slash == string::npos) ? "" : path.substr(0, last_slash);
  }

  inline void invalidate(const string &path)
  {
    if (!path.empty())
      cache::remove(path);
  }

  int touch(const string &path)
  {
    object::ptr obj;

    if (path.empty())
      return 0; // succeed if path is root

    obj = cache::get(path);

    if (!obj)
      return -ENOENT;

    obj->set_ctime();
    obj->set_mtime();

    return obj->commit();
  }

  void statistics_writer(ostream *o)
  {
    *o <<
      "operations (exceptions):\n"
      "  reopen attempts: " << s_reopen_attempts << "\n"
      "  reopens rescued: " << s_reopen_rescues << "\n"
      "  reopens failed: " << s_reopen_fails << "\n"
      "  rename attempts: " << s_rename_attempts << "\n"
      "  renames failed: " << s_rename_fails << "\n"
      "operations (modifiers):\n"
      "  create: " << s_create << "\n"
      "  mkdir: " << s_mkdir << "\n"
      "  mknod: " << s_mknod << "\n"
      "  open: " << s_open << "\n"
      "  rename: " << s_rename << "\n"
      "  symlink: " << s_symlink << "\n"
      "  truncate: " << s_truncate << "\n"
      "  unlink: " << s_unlink << "\n"
      "operations (accessors):\n"
      "  getattr: " << s_getattr << "\n"
      "  readdir: " << s_readdir << "\n"
      "  readlink: " << s_readlink << "\n";
  }

  int s_mountpoint_mode = 0;
  statistics::writers::entry s_entry(statistics_writer, 0);
}

// also adjust path by skipping leading slash
#define ASSERT_VALID_PATH(str) \
  do { \
    const char *last_slash = NULL; \
    \
    if ((str)[0] != '/') { \
      S3_LOG(LOG_WARNING, "ASSERT_VALID_PATH", "expected leading slash: [%s]\n", (str)); \
      return -EINVAL; \
    } \
    \
    if ((str)[1] != '\0' && (str)[strlen(str) - 1] == '/') { \
      S3_LOG(LOG_WARNING, "ASSERT_VALID_PATH", "invalid trailing slash: [%s]\n", (str)); \
      return -EINVAL; \
    } \
    \
    last_slash = strrchr((str), '/'); \
    \
    if (last_slash && strlen(last_slash + 1) > NAME_MAX) { \
      S3_LOG(LOG_DEBUG, "ASSERT_VALID_PATH", "final component [%s] exceeds %i characters\n", last_slash, NAME_MAX); \
      return -ENAMETOOLONG; \
    } \
    \
    (str)++; \
  } while (0)

#define CHECK_OWNER(obj) \
  do { \
    uid_t curr_uid = fuse_get_context()->uid; \
    \
    if (curr_uid && curr_uid != (obj)->get_uid()) \
      return -EPERM; \
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
  object::ptr var = cache::get(path); \
  \
  if (!var) \
    return -ENOENT;

#define GET_OBJECT_AS(type, mode, var, path) \
  type::ptr var = static_pointer_cast<type>(cache::get(path)); \
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

#define RETURN_ON_ERROR(op) \
  do { \
    int ret_val = (op); \
    \
    if (ret_val) \
      return ret_val; \
  } while (0)

void operations::init(const string &mountpoint)
{
  struct stat mp_stat;

  if (stat(mountpoint.c_str(), &mp_stat))
    throw runtime_error("failed to stat mount point.");

  s_mountpoint_mode = S_IFDIR | mp_stat.st_mode;
}

void operations::build_fuse_operations(fuse_operations *ops)
{
  memset(ops, 0, sizeof(*ops));

  ops->flag_nullpath_ok = 1;

  ops->chmod = operations::chmod;
  ops->chown = operations::chown;
  ops->create = operations::create;
  ops->getattr = operations::getattr;
  ops->getxattr = operations::getxattr;
  ops->flush = operations::flush;
  ops->ftruncate = operations::ftruncate;
  ops->listxattr = operations::listxattr;
  ops->mkdir = operations::mkdir;
  ops->mknod = operations::mknod;
  ops->open = operations::open;
  ops->read = operations::read;
  ops->readdir = operations::readdir;
  ops->readlink = operations::readlink;
  ops->release = operations::release;
  ops->removexattr = operations::removexattr;
  ops->rename = operations::rename;
  ops->rmdir = operations::unlink;
  ops->setxattr = operations::setxattr;
  ops->statfs = operations::statfs;
  ops->symlink = operations::symlink;
  ops->truncate = operations::truncate;
  ops->unlink = operations::unlink;
  ops->utimens = operations::utimens;
  ops->write = operations::write;
}

int operations::chmod(const char *path, mode_t mode)
{
  S3_LOG(LOG_DEBUG, "chmod", "path: %s, mode: %i\n", path, mode);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);
    CHECK_OWNER(obj);

    obj->set_mode(mode);

    return obj->commit();
  END_TRY;
}

int operations::chown(const char *path, uid_t uid, gid_t gid)
{
  S3_LOG(LOG_DEBUG, "chown", "path: %s, user: %i, group: %i\n", path, uid, gid);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);
    CHECK_OWNER(obj);

    if (uid != static_cast<uid_t>(-1))
      obj->set_uid(uid);

    if (gid != static_cast<gid_t>(-1))
      obj->set_gid(gid);

    // chown updates ctime
    obj->set_ctime();

    return obj->commit();
  END_TRY;
}

int operations::create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "create", "path: %s, mode: %#o\n", path, mode);
  ++s_create;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    file::ptr f;
    string parent = get_parent(path);
    int r = 0, last_error = 0;
    const fuse_context *ctx = fuse_get_context();

    if (cache::get(path)) {
      S3_LOG(LOG_WARNING, "create", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    invalidate(parent);

    if (config::get_use_encryption() && config::get_encrypt_new_files())
      f.reset(new encrypted_file(path));
    else
      f.reset(new file(path));

    f->set_mode(mode);
    f->set_uid(ctx->uid);
    f->set_gid(ctx->gid);

    RETURN_ON_ERROR(f->commit());
    RETURN_ON_ERROR(touch(parent));

    // rarely, the newly created file won't be downloadable right away, so
    // try a few times before giving up.
    for (int i = 0; i < config::get_max_inconsistent_state_retries(); i++) {
      last_error = r;
      r = file::open(static_cast<string>(path), s3::fs::OPEN_DEFAULT, &file_info->fh);

      if (r != -ENOENT)
        break;

      S3_LOG(LOG_WARNING, "create", "retrying open on [%s] because of error %i\n", path, r);
      ++s_reopen_attempts;

      // sleep a bit instead of retrying more times than necessary
      timer::sleep(i + 1);
    }

    if (!r && last_error == -ENOENT)
      ++s_reopen_rescues;

    if (r == -ENOENT)
      ++s_reopen_fails;

    return r;
  END_TRY;
}

int operations::flush(const char *path, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "flush", "path: %s\n", f->get_path().c_str());

  BEGIN_TRY;
    return f->flush();
  END_TRY;
}

int operations::ftruncate(const char *path, off_t offset, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "ftruncate", "path: %s, offset: %ji\n", f->get_path().c_str(), static_cast<intmax_t>(offset));

  BEGIN_TRY;
    RETURN_ON_ERROR(f->truncate(offset));

    // successful truncate updates ctime
    f->set_ctime();

    // we don't need to flush/commit the ctime update because that'll be done
    // when we close this file.

    return 0;
  END_TRY;
}

int operations::getattr(const char *path, struct stat *s)
{
  ASSERT_VALID_PATH(path);

  ++s_getattr;

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
int operations::getxattr(const char *path, const char *name, char *buffer, size_t max_size, uint32_t position)
#else
int operations::getxattr(const char *path, const char *name, char *buffer, size_t max_size)
#endif
{
  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    return obj->get_metadata(name, buffer, max_size);
  END_TRY;
}

int operations::listxattr(const char *path, char *buffer, size_t size)
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

int operations::mkdir(const char *path, mode_t mode)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "mkdir", "path: %s, mode: %#o\n", path, mode);
  ++s_mkdir;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    directory::ptr dir;
    string parent = get_parent(path);

    if (cache::get(path)) {
      S3_LOG(LOG_WARNING, "mkdir", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    invalidate(parent);

    dir.reset(new directory(path));

    dir->set_mode(mode);
    dir->set_uid(ctx->uid);
    dir->set_gid(ctx->gid);

    RETURN_ON_ERROR(dir->commit());

    return touch(parent);
  END_TRY;
}

int operations::mknod(const char *path, mode_t mode, dev_t dev)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "mknod", "path: %s, mode: %#o, dev: %i\n", path, mode, dev);
  ++s_mknod;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    special::ptr obj;
    string parent = get_parent(path);

    if (cache::get(path)) {
      S3_LOG(LOG_WARNING, "mknod", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    invalidate(parent);

    obj.reset(new special(path));

    obj->set_type(mode);
    obj->set_device(dev);

    obj->set_mode(mode);
    obj->set_uid(ctx->uid);
    obj->set_gid(ctx->gid);

    RETURN_ON_ERROR(obj->commit());

    return touch(parent);
  END_TRY;
}

int operations::open(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "open", "path: %s\n", path);
  ++s_open;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    RETURN_ON_ERROR(file::open(
      static_cast<string>(path), 
      (file_info->flags & O_TRUNC) ? s3::fs::OPEN_TRUNCATE_TO_ZERO : s3::fs::OPEN_DEFAULT, 
      &file_info->fh));

    // successful open with O_TRUNC updates ctime and mtime
    if (file_info->flags & O_TRUNC) {
      file::from_handle(file_info->fh)->set_ctime();
      file::from_handle(file_info->fh)->set_mtime();
    }

    return 0;
  END_TRY;
}

int operations::read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  BEGIN_TRY;
    return file::from_handle(file_info->fh)->read(buffer, size, offset);
  END_TRY;
}

int operations::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "readdir", "path: %s\n", path);
  ++s_readdir;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT_AS(directory, S_IFDIR, dir, path);

    return dir->read(bind(&dir_filler, filler, buf, _1));
  END_TRY;
}

int operations::readlink(const char *path, char *buffer, size_t max_size)
{
  S3_LOG(LOG_DEBUG, "readlink", "path: %s, max_size: %zu\n", path, max_size);
  ++s_readlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT_AS(s3::fs::symlink, S_IFLNK, link, path);

    string target;

    RETURN_ON_ERROR(link->read(&target));

    // leave room for the terminating null
    max_size--;

    if (target.size() < max_size)
      max_size = target.size();

    memcpy(buffer, target.c_str(), max_size);
    buffer[max_size] = '\0';

    return 0;
  END_TRY;
}

int operations::release(const char *path, fuse_file_info *file_info)
{
  file *f = file::from_handle(file_info->fh);

  S3_LOG(LOG_DEBUG, "release", "path: %s\n", f->get_path().c_str());

  BEGIN_TRY;
    return f->release();
  END_TRY;
}

int operations::removexattr(const char *path, const char *name)
{
  S3_LOG(LOG_DEBUG, "removexattr", "path: %s, name: %s\n", path, name);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    RETURN_ON_ERROR(obj->remove_metadata(name));

    return obj->commit();
  END_TRY;
}

int operations::rename(const char *from, const char *to)
{
  S3_LOG(LOG_DEBUG, "rename", "from: %s, to: %s\n", from, to);
  ++s_rename;

  ASSERT_VALID_PATH(from);
  ASSERT_VALID_PATH(to);

  BEGIN_TRY;
    GET_OBJECT(from_obj, from);

    // not using GET_OBJECT() here because we don't want to fail if "to"
    // doesn't exist
    object::ptr to_obj = cache::get(to);

    invalidate(get_parent(from));
    invalidate(get_parent(to));

    if (to_obj) {
      if (to_obj->get_type() == S_IFDIR) {
        if (from_obj->get_type() != S_IFDIR)
          return -EISDIR;

        if (!static_pointer_cast<directory>(to_obj)->is_empty())
          return -ENOTEMPTY;

      } else if (from_obj->get_type() == S_IFDIR) {
        return -ENOTDIR;
      }

      RETURN_ON_ERROR(to_obj->remove());
    }

    RETURN_ON_ERROR(from_obj->rename(to));

    for (int i = 0; i < config::get_max_inconsistent_state_retries(); i++) {
      to_obj = cache::get(to);

      if (to_obj)
        break;

      S3_LOG(LOG_WARNING, "rename", "newly-renamed object [%s] not available at new path\n", to);
      ++s_rename_attempts;

      // sleep a bit instead of retrying more times than necessary
      timer::sleep(i + 1);
    }

    // TODO: fail if ctime/mtime can't be set? maybe have a strict posix compliance flag?
    if (!to_obj) {
      ++s_rename_fails;
      return -EIO;
    }

    to_obj->set_ctime();

    return to_obj->commit();
  END_TRY;
}

#ifdef __APPLE__
int operations::setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
#else
int operations::setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
#endif
{
  S3_LOG(LOG_DEBUG, "setxattr", "path: [%s], name: [%s], size: %i\n", path, name, size);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    bool needs_commit = false;
    GET_OBJECT(obj, path);

    RETURN_ON_ERROR(obj->set_metadata(name, value, size, flags, &needs_commit));

    return needs_commit ? obj->commit() : 0;
  END_TRY;
}

int operations::statfs(const char * /* ignored */, struct statvfs *s)
{
  s->f_namemax = 1024; // arbitrary

  s->f_bsize = object::get_block_size();

  // the following members are all 64-bit, but on 32-bit systems we want to 
  // return values that'll fit in 32 bits, otherwise we screw up tools like
  // "df". hence "size_t" rather than the type of the member.

  s->f_blocks = numeric_limits<size_t>::max();
  s->f_bfree = numeric_limits<size_t>::max();
  s->f_bavail = numeric_limits<size_t>::max();
  s->f_files = numeric_limits<size_t>::max();
  s->f_ffree = numeric_limits<size_t>::max();

  return 0;
}

int operations::symlink(const char *target, const char *path)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "symlink", "path: %s, target: %s\n", path, target);
  ++s_symlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    symlink::ptr link;
    string parent = get_parent(path);

    if (cache::get(path)) {
      S3_LOG(LOG_WARNING, "symlink", "attempt to overwrite object at [%s]\n", path);
      return -EEXIST;
    }

    invalidate(parent);

    link.reset(new s3::fs::symlink(path));

    link->set_uid(ctx->uid);
    link->set_gid(ctx->gid);

    link->set_target(target);

    RETURN_ON_ERROR(link->commit());

    return touch(parent);
  END_TRY;
}

int operations::truncate(const char *path, off_t size)
{
  file *f;
  uint64_t handle;

  S3_LOG(LOG_DEBUG, "truncate", "path: %s, size: %ji\n", path, static_cast<intmax_t>(size));
  ++s_truncate;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    int r;

    // passing OPEN_TRUNCATE_TO_ZERO saves us from having to download the 
    // entire file if we're just going to truncate it to zero anyway.

    RETURN_ON_ERROR(file::open(
      static_cast<string>(path), 
      (size == 0) ? s3::fs::OPEN_TRUNCATE_TO_ZERO : s3::fs::OPEN_DEFAULT,
      &handle));

    f = file::from_handle(handle);

    r = f->truncate(size);

    if (!r) {
      // successful truncate updates ctime
      f->set_ctime();

      r = f->flush();
    }

    f->release();

    return r;
  END_TRY;
}

int operations::unlink(const char *path)
{
  S3_LOG(LOG_DEBUG, "unlink", "path: %s\n", path);
  ++s_unlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    string parent = get_parent(path);

    GET_OBJECT(obj, path);

    invalidate(parent);

    RETURN_ON_ERROR(obj->remove());

    return touch(parent);
  END_TRY;
}

int operations::utimens(const char *path, const timespec times[2])
{
  S3_LOG(LOG_DEBUG, "utimens", "path: %s, time: %li\n", path, times[1].tv_sec);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
    GET_OBJECT(obj, path);

    obj->set_mtime(times[1].tv_sec);

    return obj->commit();
  END_TRY;
}

int operations::write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  BEGIN_TRY;
    return file::from_handle(file_info->fh)->write(buffer, size, offset);
  END_TRY;
}
