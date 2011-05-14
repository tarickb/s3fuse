#include "logging.hh"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <pugixml/pugixml.hpp>

#include "fs.hh"

using namespace std;

namespace
{
  int g_mountpoint_mode = 0755;

  s3::fs *g_fs = NULL;
}

#define ASSERT_LEADING_SLASH(str) do { if ((str)[0] != '/') return -EINVAL; } while (0)

int s3_getattr(const char *path, struct stat *s)
{
  // TODO: far too verbose
  // S3_DEBUG("s3_getattr", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  if (strcmp(path, "/") == 0) {
    s->st_mode = S_IFDIR | g_mountpoint_mode;
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  return g_fs->get_stats(path + 1, s);
}

int s3_chown(const char *path, uid_t uid, gid_t gid)
{
  S3_DEBUG("s3_chown", "path: %s, user: %i, group: %i\n", path, uid, gid);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_owner(path + 1, uid, gid);
}

int s3_chmod(const char *path, mode_t mode)
{
  S3_DEBUG("s3_chmod", "path: %s, mode: %i\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_mode(path + 1, mode);
}

int s3_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_DEBUG("s3_readdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->read_directory(path + 1, filler, buf);
}

int s3_mkdir(const char *path, mode_t mode)
{
  S3_DEBUG("s3_mkdir", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return g_fs->create_directory(path + 1, mode);
}

int s3_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  int r;

  S3_DEBUG("s3_create", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  r = g_fs->create_file(path + 1, mode);

  if (r)
    return r;

  return g_fs->open(path + 1, &file_info->fh);
}

int s3_open(const char *path, fuse_file_info *file_info)
{
  S3_DEBUG("s3_open", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->open(path + 1, &file_info->fh);
}

int s3_read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return g_fs->read(file_info->fh, buffer, size, offset);
}

int s3_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return g_fs->write(file_info->fh, buffer, size, offset);
}

int s3_flush(const char *path, fuse_file_info *file_info)
{
  S3_DEBUG("s3_flush", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->flush(file_info->fh);
}

int s3_release(const char *path, fuse_file_info *file_info)
{
  S3_DEBUG("s3_release", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->release(file_info->fh);
}

int s3_access(const char *path, int mode)
{
  S3_DEBUG("s3_access", "path: %s, mask: %i\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  // TODO: check access
  return 0;
}

int s3_truncate(const char *path, off_t offset)
{
  S3_DEBUG("s3_truncate", "path: %s, offset: %ji\n", path, static_cast<intmax_t>(offset));
  ASSERT_LEADING_SLASH(path);

  return g_fs->truncate(path + 1, offset);
}

int s3_unlink(const char *path)
{
  S3_DEBUG("s3_unlink", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->remove_file(path + 1);
}

int s3_rmdir(const char *path)
{
  S3_DEBUG("s3_rmdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->remove_directory(path + 1);
}

int s3_rename(const char *from, const char *to)
{
  S3_DEBUG("s3_rename", "from: %s, to: %s\n", from, to);
  ASSERT_LEADING_SLASH(from);
  ASSERT_LEADING_SLASH(to);

  return g_fs->rename_object(from + 1, to + 1);
}

int s3_utimens(const char *path, const timespec times[2])
{
  S3_DEBUG("s3_utimens", "path: %s, time: %li\n", path, times[1].tv_sec);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_mtime(path + 1, times[1].tv_sec);
}

int s3_symlink(const char *target, const char *path)
{
  S3_DEBUG("s3_symlink", "path: %s, target: %s\n", path, target);
  ASSERT_LEADING_SLASH(path);

  return g_fs->create_symlink(path + 1, target);
}

int s3_readlink(const char *path, char *buffer, size_t max_size)
{
  string target;
  int r;

  S3_DEBUG("s3_readlink", "path: %s, max_size: %zu\n", path, max_size);
  ASSERT_LEADING_SLASH(path);

  r = g_fs->read_symlink(path + 1, &target);

  if (r)
    return r;

  if (target.size() < max_size)
    max_size = target.size();

  target.copy(buffer, max_size);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  fuse_operations opers;

  memset(&opers, 0, sizeof(opers));

  opers.access = s3_access;
  opers.chmod = s3_chmod;
  opers.chown = s3_chown;
  opers.create = s3_create;
  opers.getattr = s3_getattr;
  opers.flush = s3_flush;
  opers.mkdir = s3_mkdir;
  opers.open = s3_open;
  opers.read = s3_read;
  opers.readdir = s3_readdir;
  opers.readlink = s3_readlink;
  opers.release = s3_release;
  opers.rename = s3_rename;
  opers.rmdir = s3_rmdir;
  opers.symlink = s3_symlink;
  opers.truncate = s3_truncate;
  opers.unlink = s3_unlink;
  opers.utimens = s3_utimens;
  opers.write = s3_write;

  g_fs = new s3::fs();

  r = fuse_main(argc, argv, &opers, NULL);

  delete g_fs;
  return r;
}
