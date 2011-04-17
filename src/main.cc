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

int s3_getattr(const char *path, struct stat *s)
{
  S3_DEBUG("s3_getattr", "path: %s\n", path);

  if (strcmp(path, "/") == 0) {
    s->st_mode = S_IFDIR | g_mountpoint_mode;
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  if (path[0] == '/')
    path++;

  return g_fs->get_stats(path, s);
}

int s3_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_DEBUG("s3_readdir", "path: %s\n", path);

  if (path[0] == '/')
    path++;

  return g_fs->read_directory(path, filler, buf);
}

int s3_open(const char *path, fuse_file_info *file_info)
{
  S3_DEBUG("s3_open", "path: %s\n", path);
}

int s3_read(const char *path, char *, size_t, off_t, fuse_file_info *file_info)
{
  S3_DEBUG("s3_read", "path: %s\n", path);
}

int main(int argc, char **argv)
{
  fuse_operations opers;

  memset(&opers, 0, sizeof(opers));

  opers.getattr = s3_getattr;
  opers.readdir = s3_readdir;
  opers.open = s3_open;
  opers.read = s3_read;

  g_fs = new s3::fs("test-0");

  return fuse_main(argc, argv, &opers, NULL);
}
