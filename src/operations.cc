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

#include "operations.h"

#include <atomic>
#include <limits>

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

namespace s3 {

namespace {
std::atomic_int s_reopen_attempts(0), s_reopen_rescues(0), s_reopen_fails(0);
std::atomic_int s_rename_attempts(0), s_rename_fails(0);
std::atomic_int s_create(0), s_mkdir(0), s_mknod(0), s_open(0), s_rename(0),
    s_symlink(0), s_truncate(0), s_unlink(0);
std::atomic_int s_getattr(0), s_readdir(0), s_readlink(0);

inline std::string GetParent(const std::string &path) {
  size_t last_slash = path.rfind('/');
  return (last_slash == std::string::npos) ? "" : path.substr(0, last_slash);
}

inline void Invalidate(const std::string &path) {
  if (!path.empty()) fs::Cache::Remove(path);
}

int Touch(const std::string &path) {
  if (path.empty()) return 0;  // succeed if path is root

  auto obj = fs::Cache::Get(path);
  if (!obj) return -ENOENT;

  obj->set_ctime();
  obj->set_mtime();

  return obj->Commit();
}

void StatsWriter(std::ostream *o) {
  *o << "operations (exceptions):\n"
        "  reopen attempts: "
     << s_reopen_attempts
     << "\n"
        "  reopens rescued: "
     << s_reopen_rescues
     << "\n"
        "  reopens failed: "
     << s_reopen_fails
     << "\n"
        "  rename attempts: "
     << s_rename_attempts
     << "\n"
        "  renames failed: "
     << s_rename_fails
     << "\n"
        "operations (modifiers):\n"
        "  create: "
     << s_create
     << "\n"
        "  mkdir: "
     << s_mkdir
     << "\n"
        "  mknod: "
     << s_mknod
     << "\n"
        "  open: "
     << s_open
     << "\n"
        "  rename: "
     << s_rename
     << "\n"
        "  symlink: "
     << s_symlink
     << "\n"
        "  truncate: "
     << s_truncate
     << "\n"
        "  unlink: "
     << s_unlink
     << "\n"
        "operations (accessors):\n"
        "  getattr: "
     << s_getattr
     << "\n"
        "  readdir: "
     << s_readdir
     << "\n"
        "  readlink: "
     << s_readlink << "\n";
}

int s_mountpoint_mode = 0;
base::Statistics::Writers::Entry s_entry(StatsWriter, 0);
}  // namespace

// also adjust path by skipping leading slash
#define ASSERT_VALID_PATH(str)                                           \
  do {                                                                   \
    const char *last_slash = nullptr;                                    \
                                                                         \
    if ((str)[0] != '/') {                                               \
      S3_LOG(LOG_WARNING, "ASSERT_VALID_PATH",                           \
             "expected leading slash: [%s]\n", (str));                   \
      return -EINVAL;                                                    \
    }                                                                    \
                                                                         \
    if ((str)[1] != '\0' && (str)[strlen(str) - 1] == '/') {             \
      S3_LOG(LOG_WARNING, "ASSERT_VALID_PATH",                           \
             "invalid trailing slash: [%s]\n", (str));                   \
      return -EINVAL;                                                    \
    }                                                                    \
                                                                         \
    last_slash = strrchr((str), '/');                                    \
                                                                         \
    if (last_slash && strlen(last_slash + 1) > NAME_MAX) {               \
      S3_LOG(LOG_DEBUG, "ASSERT_VALID_PATH",                             \
             "final component [%s] exceeds %i characters\n", last_slash, \
             NAME_MAX);                                                  \
      return -ENAMETOOLONG;                                              \
    }                                                                    \
                                                                         \
    (str)++;                                                             \
  } while (0)

#define CHECK_OWNER(obj)                                     \
  do {                                                       \
    uid_t curr_uid = fuse_get_context()->uid;                \
                                                             \
    if (curr_uid && curr_uid != (obj)->uid()) return -EPERM; \
  } while (0)

#define BEGIN_TRY try {
#define END_TRY                                                               \
  }                                                                           \
  catch (const std::exception &e) {                                           \
    S3_LOG(LOG_WARNING, "END_TRY", "caught exception: %s (at line %i)\n",     \
           e.what(), __LINE__);                                               \
    return -ECANCELED;                                                        \
  }                                                                           \
  catch (...) {                                                               \
    S3_LOG(LOG_WARNING, "END_TRY", "caught unknown exception (at line %i)\n", \
           __LINE__);                                                         \
    return -ECANCELED;                                                        \
  }

#define GET_OBJECT(var, path)      \
  auto var = fs::Cache::Get(path); \
  if (!var) return -ENOENT;

#define GET_OBJECT_AS(obj_type, mode, var, path)                           \
  auto var = std::dynamic_pointer_cast<obj_type>(fs::Cache::Get(path));    \
  if (!var) return -ENOENT;                                                \
  if (var->type() != (mode)) {                                             \
    S3_LOG(LOG_WARNING, "GET_OBJECT_AS",                                   \
           "could not get [%s] as type [%s] (requested mode %i, reported " \
           "mode %i, at line %i)\n",                                       \
           static_cast<const char *>(path), #obj_type, mode, var->type(),  \
           __LINE__);                                                      \
    return -EINVAL;                                                        \
  }

#define RETURN_ON_ERROR(op)      \
  do {                           \
    int ret_val = (op);          \
    if (ret_val) return ret_val; \
  } while (0)

void Operations::Init(const std::string &mountpoint) {
  struct stat mp_stat;

  if (stat(mountpoint.c_str(), &mp_stat))
    throw std::runtime_error("failed to stat mount point.");

  s_mountpoint_mode = S_IFDIR | mp_stat.st_mode;
}

void Operations::BuildFuseOperations(fuse_operations *ops) {
  memset(ops, 0, sizeof(*ops));

  ops->flag_nullpath_ok = 1;

  ops->chmod = Operations::chmod;
  ops->chown = Operations::chown;
  ops->create = Operations::create;
  ops->getattr = Operations::getattr;
  ops->getxattr = Operations::getxattr;
  ops->flush = Operations::flush;
  ops->ftruncate = Operations::ftruncate;
  ops->listxattr = Operations::listxattr;
  ops->mkdir = Operations::mkdir;
  ops->mknod = Operations::mknod;
  ops->open = Operations::open;
  ops->read = Operations::read;
  ops->readdir = Operations::readdir;
  ops->readlink = Operations::readlink;
  ops->release = Operations::release;
  ops->removexattr = Operations::removexattr;
  ops->rename = Operations::rename;
  ops->rmdir = Operations::unlink;
  ops->setxattr = Operations::setxattr;
  ops->statfs = Operations::statfs;
  ops->symlink = Operations::symlink;
  ops->truncate = Operations::truncate;
  ops->unlink = Operations::unlink;
  ops->utimens = Operations::utimens;
  ops->write = Operations::write;
}

int Operations::chmod(const char *path, mode_t mode) {
  S3_LOG(LOG_DEBUG, "chmod", "path: %s, mode: %i\n", path, mode);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  GET_OBJECT(obj, path);
  CHECK_OWNER(obj);

  obj->SetMode(mode);

  return obj->Commit();

  END_TRY;
}

int Operations::chown(const char *path, uid_t uid, gid_t gid) {
  S3_LOG(LOG_DEBUG, "chown", "path: %s, user: %i, group: %i\n", path, uid, gid);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  GET_OBJECT(obj, path);
  CHECK_OWNER(obj);

  if (uid != static_cast<uid_t>(-1)) obj->set_uid(uid);
  if (gid != static_cast<gid_t>(-1)) obj->set_gid(gid);
  // chown updates ctime
  obj->set_ctime();

  return obj->Commit();

  END_TRY;
}

int Operations::create(const char *path, mode_t mode,
                       fuse_file_info *file_info) {
  S3_LOG(LOG_DEBUG, "create", "path: %s, mode: %#o\n", path, mode);
  ++s_create;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  if (fs::Cache::Get(path)) {
    S3_LOG(LOG_WARNING, "create", "attempt to overwrite object at [%s]\n",
           path);
    return -EEXIST;
  }

  std::string parent = GetParent(path);
  Invalidate(parent);

  std::unique_ptr<fs::File> f;

  if (base::Config::use_encryption() && base::Config::encrypt_new_files())
    f.reset(new fs::EncryptedFile(path));
  else
    f.reset(new fs::File(path));

  f->SetMode(mode);
  f->set_uid(fuse_get_context()->uid);
  f->set_gid(fuse_get_context()->gid);

  RETURN_ON_ERROR(f->Commit());
  RETURN_ON_ERROR(Touch(parent));

  // rarely, the newly created file won't be downloadable right away, so
  // try a few times before giving up.
  int r = 0, last_error = 0;
  for (int i = 0; i < base::Config::max_inconsistent_state_retries(); i++) {
    last_error = r;
    r = fs::File::Open(static_cast<std::string>(path),
                       s3::fs::FileOpenMode::DEFAULT, &file_info->fh);
    if (r != -ENOENT) break;
    S3_LOG(LOG_WARNING, "create", "retrying open on [%s] because of error %i\n",
           path, r);
    ++s_reopen_attempts;
    // sleep a bit instead of retrying more times than necessary
    base::Timer::Sleep(i + 1);
  }

  if (!r && last_error == -ENOENT) ++s_reopen_rescues;
  if (r == -ENOENT) ++s_reopen_fails;

  return r;

  END_TRY;
}

int Operations::flush(const char *path, fuse_file_info *file_info) {
  auto *f = fs::File::FromHandle(file_info->fh);

  S3_LOG(LOG_DEBUG, "flush", "path: %s\n", f->path().c_str());

  BEGIN_TRY;
  return f->Flush();
  END_TRY;
}

int Operations::ftruncate(const char *path, off_t offset,
                          fuse_file_info *file_info) {
  auto *f = fs::File::FromHandle(file_info->fh);

  S3_LOG(LOG_DEBUG, "ftruncate", "path: %s, offset: %ji\n", f->path().c_str(),
         static_cast<intmax_t>(offset));

  BEGIN_TRY;
  RETURN_ON_ERROR(f->Truncate(offset));
  // successful truncate updates ctime
  f->set_ctime();
  // we don't need to flush/commit the ctime update because that'll be done
  // when we close this file.
  return 0;
  END_TRY;
}

int Operations::getattr(const char *path, struct stat *s) {
  ASSERT_VALID_PATH(path);

  ++s_getattr;

  memset(s, 0, sizeof(*s));

  if (path[0] == '\0') {  // root path
    s->st_uid = geteuid();
    s->st_gid = getegid();
    s->st_mode = s_mountpoint_mode;
    s->st_nlink = 1;  // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  BEGIN_TRY;

  GET_OBJECT(obj, path);

  obj->CopyStat(s);

  return 0;

  END_TRY;
}

#ifdef __APPLE__
int Operations::getxattr(const char *path, const char *name, char *buffer,
                         size_t max_size, uint32_t position)
#else
int Operations::getxattr(const char *path, const char *name, char *buffer,
                         size_t max_size)
#endif
{
  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
  GET_OBJECT(obj, path);
  return obj->GetMetadata(name, buffer, max_size);
  END_TRY;
}

int Operations::listxattr(const char *path, char *buffer, size_t size) {
  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  GET_OBJECT(obj, path);

  const auto attrs = obj->GetMetadataKeys();

  size_t required_size = 0;
  for (const auto &attr : attrs) required_size += attr.size() + 1;

  if (buffer == nullptr || size == 0) return required_size;
  if (required_size > size) return -ERANGE;

  for (const auto &attr : attrs) {
    strcpy(buffer, attr.c_str());
    buffer += attr.size() + 1;
  }

  return required_size;

  END_TRY;
}

int Operations::mkdir(const char *path, mode_t mode) {
  S3_LOG(LOG_DEBUG, "mkdir", "path: %s, mode: %#o\n", path, mode);
  ++s_mkdir;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  if (fs::Cache::Get(path)) {
    S3_LOG(LOG_WARNING, "mkdir", "attempt to overwrite object at [%s]\n", path);
    return -EEXIST;
  }

  std::string parent = GetParent(path);
  Invalidate(parent);

  fs::Directory dir(path);

  dir.SetMode(mode);
  dir.set_uid(fuse_get_context()->uid);
  dir.set_gid(fuse_get_context()->gid);

  RETURN_ON_ERROR(dir.Commit());

  return Touch(parent);

  END_TRY;
}

int Operations::mknod(const char *path, mode_t mode, dev_t dev) {
  S3_LOG(LOG_DEBUG, "mknod", "path: %s, mode: %#o, dev: %i\n", path, mode, dev);
  ++s_mknod;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  if (fs::Cache::Get(path)) {
    S3_LOG(LOG_WARNING, "mknod", "attempt to overwrite object at [%s]\n", path);
    return -EEXIST;
  }

  std::string parent = GetParent(path);
  Invalidate(parent);

  fs::Special obj(path);

  obj.set_type(mode);
  obj.set_device(dev);

  obj.SetMode(mode);
  obj.set_uid(fuse_get_context()->uid);
  obj.set_gid(fuse_get_context()->gid);

  RETURN_ON_ERROR(obj.Commit());

  return Touch(parent);

  END_TRY;
}

int Operations::open(const char *path, fuse_file_info *file_info) {
  S3_LOG(LOG_DEBUG, "open", "path: %s\n", path);
  ++s_open;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  RETURN_ON_ERROR(fs::File::Open(static_cast<std::string>(path),
                                 (file_info->flags & O_TRUNC)
                                     ? s3::fs::FileOpenMode::TRUNCATE_TO_ZERO
                                     : s3::fs::FileOpenMode::DEFAULT,
                                 &file_info->fh));

  // successful open with O_TRUNC updates ctime and mtime
  if (file_info->flags & O_TRUNC) {
    fs::File::FromHandle(file_info->fh)->set_ctime();
    fs::File::FromHandle(file_info->fh)->set_mtime();
  }

  return 0;

  END_TRY;
}

int Operations::read(const char *path, char *buffer, size_t size, off_t offset,
                     fuse_file_info *file_info) {
  BEGIN_TRY;
  return fs::File::FromHandle(file_info->fh)->Read(buffer, size, offset);
  END_TRY;
}

int Operations::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t, fuse_file_info *file_info) {
  S3_LOG(LOG_DEBUG, "readdir", "path: %s\n", path);
  ++s_readdir;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
  GET_OBJECT_AS(fs::Directory, S_IFDIR, dir, path);
  return dir->Read([filler, buf](const std::string &path) {
    filler(buf, path.c_str(), nullptr, 0);
  });
  END_TRY;
}

int Operations::readlink(const char *path, char *buffer, size_t max_size) {
  S3_LOG(LOG_DEBUG, "readlink", "path: %s, max_size: %zu\n", path, max_size);
  ++s_readlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  GET_OBJECT_AS(fs::Symlink, S_IFLNK, link, path);

  std::string target;
  RETURN_ON_ERROR(link->Read(&target));

  // leave room for the terminating null
  max_size--;
  if (target.size() < max_size) max_size = target.size();
  memcpy(buffer, target.c_str(), max_size);
  buffer[max_size] = '\0';

  return 0;

  END_TRY;
}

int Operations::release(const char *path, fuse_file_info *file_info) {
  auto *f = fs::File::FromHandle(file_info->fh);

  S3_LOG(LOG_DEBUG, "release", "path: %s\n", f->path().c_str());

  BEGIN_TRY;
  return f->Release();
  END_TRY;
}

int Operations::removexattr(const char *path, const char *name) {
  S3_LOG(LOG_DEBUG, "removexattr", "path: %s, name: %s\n", path, name);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
  GET_OBJECT(obj, path);
  RETURN_ON_ERROR(obj->RemoveMetadata(name));
  return obj->Commit();
  END_TRY;
}

int Operations::rename(const char *from, const char *to) {
  S3_LOG(LOG_DEBUG, "rename", "from: %s, to: %s\n", from, to);
  ++s_rename;

  ASSERT_VALID_PATH(from);
  ASSERT_VALID_PATH(to);

  BEGIN_TRY;

  GET_OBJECT(from_obj, from);

  // not using GET_OBJECT() here because we don't want to fail if "to"
  // doesn't exist
  auto to_obj = fs::Cache::Get(to);

  Invalidate(GetParent(from));
  Invalidate(GetParent(to));

  if (to_obj) {
    if (to_obj->type() == S_IFDIR) {
      if (from_obj->type() != S_IFDIR) return -EISDIR;
      if (!std::dynamic_pointer_cast<fs::Directory>(to_obj)->IsEmpty())
        return -ENOTEMPTY;
    } else if (from_obj->type() == S_IFDIR) {
      return -ENOTDIR;
    }
    RETURN_ON_ERROR(to_obj->Remove());
  }

  RETURN_ON_ERROR(from_obj->Rename(to));

  for (int i = 0; i < base::Config::max_inconsistent_state_retries(); i++) {
    to_obj = fs::Cache::Get(to);
    if (to_obj) break;

    S3_LOG(LOG_WARNING, "rename",
           "newly-renamed object [%s] not available at new path\n", to);
    ++s_rename_attempts;

    // sleep a bit instead of retrying more times than necessary
    base::Timer::Sleep(i + 1);
  }

  // TODO: fail if ctime/mtime can't be set? maybe have a strict posix
  // compliance flag?
  if (!to_obj) {
    ++s_rename_fails;
    return -EIO;
  }

  to_obj->set_ctime();

  return to_obj->Commit();

  END_TRY;
}

#ifdef __APPLE__
int Operations::setxattr(const char *path, const char *name, const char *value,
                         size_t size, int flags, uint32_t position)
#else
int Operations::setxattr(const char *path, const char *name, const char *value,
                         size_t size, int flags)
#endif
{
  S3_LOG(LOG_DEBUG, "setxattr", "path: [%s], name: [%s], size: %i\n", path,
         name, size);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
  bool needs_commit = false;
  GET_OBJECT(obj, path);
  RETURN_ON_ERROR(obj->SetMetadata(name, value, size, flags, &needs_commit));
  return needs_commit ? obj->Commit() : 0;
  END_TRY;
}

int Operations::statfs(const char * /* ignored */, struct statvfs *s) {
  s->f_namemax = 1024;  // arbitrary

  s->f_bsize = fs::Object::block_size();

  // the following members are all 64-bit, but on 32-bit systems we want to
  // return values that'll fit in 32 bits, otherwise we screw up tools like
  // "df". hence "size_t" rather than the type of the member.

  s->f_blocks = std::numeric_limits<decltype(s->f_blocks)>::max();
  s->f_bfree = std::numeric_limits<decltype(s->f_bfree)>::max();
  s->f_bavail = std::numeric_limits<decltype(s->f_bavail)>::max();
  s->f_files = std::numeric_limits<decltype(s->f_files)>::max();
  s->f_ffree = std::numeric_limits<decltype(s->f_ffree)>::max();

  return 0;
}

int Operations::symlink(const char *target, const char *path) {
  S3_LOG(LOG_DEBUG, "symlink", "path: %s, target: %s\n", path, target);
  ++s_symlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  if (fs::Cache::Get(path)) {
    S3_LOG(LOG_WARNING, "symlink", "attempt to overwrite object at [%s]\n",
           path);
    return -EEXIST;
  }

  std::string parent = GetParent(path);
  Invalidate(parent);

  fs::Symlink link(path);

  link.set_uid(fuse_get_context()->uid);
  link.set_gid(fuse_get_context()->gid);
  link.SetTarget(target);
  RETURN_ON_ERROR(link.Commit());

  return Touch(parent);

  END_TRY;
}

int Operations::truncate(const char *path, off_t size) {
  S3_LOG(LOG_DEBUG, "truncate", "path: %s, size: %ji\n", path,
         static_cast<intmax_t>(size));
  ++s_truncate;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  // passing OPEN_TRUNCATE_TO_ZERO saves us from having to download the
  // entire file if we're just going to truncate it to zero anyway.
  uint64_t handle;
  RETURN_ON_ERROR(fs::File::Open(static_cast<std::string>(path),
                                 (size == 0)
                                     ? s3::fs::FileOpenMode::TRUNCATE_TO_ZERO
                                     : s3::fs::FileOpenMode::DEFAULT,
                                 &handle));

  auto *f = fs::File::FromHandle(handle);
  int r = f->Truncate(size);
  if (!r) {
    // successful truncate updates ctime
    f->set_ctime();
    r = f->Flush();
  }
  f->Release();
  return r;

  END_TRY;
}

int Operations::unlink(const char *path) {
  S3_LOG(LOG_DEBUG, "unlink", "path: %s\n", path);
  ++s_unlink;

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;

  GET_OBJECT(obj, path);

  std::string parent = GetParent(path);
  Invalidate(parent);

  RETURN_ON_ERROR(obj->Remove());

  return Touch(parent);

  END_TRY;
}

int Operations::utimens(const char *path, const timespec times[2]) {
  S3_LOG(LOG_DEBUG, "utimens", "path: %s, time: %li\n", path, times[1].tv_sec);

  ASSERT_VALID_PATH(path);

  BEGIN_TRY;
  GET_OBJECT(obj, path);
  obj->set_mtime(times[1].tv_sec);
  return obj->Commit();
  END_TRY;
}

int Operations::write(const char *path, const char *buffer, size_t size,
                      off_t offset, fuse_file_info *file_info) {
  BEGIN_TRY;
  return fs::File::FromHandle(file_info->fh)->Write(buffer, size, offset);
  END_TRY;
}

}  // namespace s3
