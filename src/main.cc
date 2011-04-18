#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <pugixml/pugixml.hpp>

#include "s3_debug.hh"
#include "s3_fs.hh"

namespace
{
  int g_mountpoint_mode = 0755;

  s3::fs *g_fs = NULL;
}

#define ASSERT_LEADING_SLASH(str) do { if ((str)[0] != '/') return -EINVAL; } while (0)

int s3_getattr(const char *path, struct stat *s)
{
  S3_DEBUG("s3_getattr", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  if (strcmp(path, "/") == 0) {
    s->st_mode = S_IFDIR | g_mountpoint_mode;
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  return g_fs->get_stats(path + 1, s);
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

  return g_fs->create_object(path + 1, mode | S_IFDIR);
}

int s3_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  S3_DEBUG("s3_create", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return g_fs->create_object(path + 1, mode | S_IFREG);
}

int s3_open(const char *path, fuse_file_info *file_info)
{
  S3_DEBUG("s3_open", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);
}

int s3_read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  S3_DEBUG("s3_read", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  int r = pread(file_info->fh, buffer, size, offset);

  return (r == -1) ? -errno : r;
}

int s3_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  S3_DEBUG("s3_write", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  int r = pwrite(file_info->fh, buffer, size, offset);

  return (r == -1) ? -errno : r;
}

int main(int argc, char **argv)
{
  fuse_operations opers;

  memset(&opers, 0, sizeof(opers));

  opers.create = s3_create;
  opers.getattr = s3_getattr;
  opers.mkdir = s3_mkdir;
  opers.open = s3_open;
  opers.read = s3_read;
  opers.readdir = s3_readdir;
  opers.write = s3_write;

  g_fs = new s3::fs("test-0");

  return fuse_main(argc, argv, &opers, NULL);
}
