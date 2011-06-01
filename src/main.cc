#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "config.h"
#include "fs.h"
#include "logger.h"
#include "version.h"

using namespace std;

namespace
{
  struct options
  {
    const char *arg0;
    string config;
    int verbosity;
    bool have_mountpoint;
  };

  s3::fs *g_fs = NULL;
}

#define ASSERT_LEADING_SLASH(str) do { if ((str)[0] != '/') return -EINVAL; } while (0)

int wrap_getattr(const char *path, struct stat *s)
{
  ASSERT_LEADING_SLASH(path);

  if (strcmp(path, "/") == 0) {
    s->st_mode = S_IFDIR | s3::config::get_mountpoint_mode();
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  return g_fs->get_stats(path + 1, s);
}

int wrap_chown(const char *path, uid_t uid, gid_t gid)
{
  S3_LOG(LOG_DEBUG, "chown", "path: %s, user: %i, group: %i\n", path, uid, gid);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_owner(path + 1, uid, gid);
}

int wrap_chmod(const char *path, mode_t mode)
{
  S3_LOG(LOG_DEBUG, "chmod", "path: %s, mode: %i\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_mode(path + 1, mode);
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "readdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->read_directory(path + 1, filler, buf);
}

int wrap_mkdir(const char *path, mode_t mode)
{
  S3_LOG(LOG_DEBUG, "mkdir", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return g_fs->create_directory(path + 1, mode);
}

int wrap_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  int r;

  S3_LOG(LOG_DEBUG, "create", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  r = g_fs->create_file(path + 1, mode);

  if (r)
    return r;

  return g_fs->open(path + 1, &file_info->fh);
}

int wrap_open(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "open", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->open(path + 1, &file_info->fh);
}

int wrap_read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return g_fs->read(file_info->fh, buffer, size, offset);
}

int wrap_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return g_fs->write(file_info->fh, buffer, size, offset);
}

int wrap_flush(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "flush", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->flush(file_info->fh);
}

int wrap_release(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "release", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->release(file_info->fh);
}

int wrap_access(const char *path, int mode)
{
  S3_LOG(LOG_DEBUG, "access", "path: %s, mask: %i\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  // TODO: check access
  return 0;
}

int wrap_truncate(const char *path, off_t offset)
{
  S3_LOG(LOG_DEBUG, "truncate", "path: %s, offset: %ji\n", path, static_cast<intmax_t>(offset));
  ASSERT_LEADING_SLASH(path);

  return g_fs->truncate(path + 1, offset);
}

int wrap_unlink(const char *path)
{
  S3_LOG(LOG_DEBUG, "unlink", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->remove_file(path + 1);
}

int wrap_rmdir(const char *path)
{
  S3_LOG(LOG_DEBUG, "rmdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return g_fs->remove_directory(path + 1);
}

int wrap_rename(const char *from, const char *to)
{
  S3_LOG(LOG_DEBUG, "rename", "from: %s, to: %s\n", from, to);
  ASSERT_LEADING_SLASH(from);
  ASSERT_LEADING_SLASH(to);

  return g_fs->rename_object(from + 1, to + 1);
}

int wrap_utimens(const char *path, const timespec times[2])
{
  S3_LOG(LOG_DEBUG, "utimens", "path: %s, time: %li\n", path, times[1].tv_sec);
  ASSERT_LEADING_SLASH(path);

  return g_fs->change_mtime(path + 1, times[1].tv_sec);
}

int wrap_symlink(const char *target, const char *path)
{
  S3_LOG(LOG_DEBUG, "symlink", "path: %s, target: %s\n", path, target);
  ASSERT_LEADING_SLASH(path);

  return g_fs->create_symlink(path + 1, target);
}

int wrap_readlink(const char *path, char *buffer, size_t max_size)
{
  string target;
  int r;

  S3_LOG(LOG_DEBUG, "readlink", "path: %s, max_size: %zu\n", path, max_size);
  ASSERT_LEADING_SLASH(path);

  r = g_fs->read_symlink(path + 1, &target);

  if (r)
    return r;

  // leave room for the terminating null
  max_size--;

  if (target.size() < max_size)
    max_size = target.size();

  memcpy(buffer, target.c_str(), max_size);
  buffer[max_size] = '\0';

  return 0;
}

int wrap_getxattr(const char *path, const char *name, char *buffer, size_t max_size)
{
  string attr;
  int r;

  ASSERT_LEADING_SLASH(path);

  r = g_fs->get_attr(path + 1, name, &attr);

  if (r)
    return r;

  if (max_size == 0)
    return attr.size() + 1;

  // leave room for the terminating null
  max_size--;

  if (attr.size() < max_size)
    max_size = attr.size();

  memcpy(buffer, attr.c_str(), max_size);
  buffer[max_size] = '\0';

  return max_size + 1; // +1 for \0
}

int wrap_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
  S3_LOG(LOG_DEBUG, "setxattr", "path: %s, name: %s, value: %s\n", path, name, value);
  ASSERT_LEADING_SLASH(path);

  if (value[size - 1] != '\0')
    return -EINVAL; // require null-terminated string for "value"

  return g_fs->set_attr(path + 1, name, value, flags);
}

int wrap_listxattr(const char *path, char *buffer, size_t size)
{
  vector<string> attrs;
  size_t required_size = 0;
  int r;

  S3_LOG(LOG_DEBUG, "listxattr", "path: %s, size: %zu\n", path, size);
  ASSERT_LEADING_SLASH(path);

  r = g_fs->list_attr(path + 1, &attrs);

  if (r)
    return r;

  for (size_t i = 0; i < attrs.size(); i++)
    required_size += attrs[i].size() + 1;

  if (size == 0)
    return required_size;

  if (required_size > size)
    return -ERANGE;

  for (size_t i = 0; i < attrs.size(); i++) {
    strcpy(buffer, attrs[i].c_str());
    buffer += attrs[i].size() + 1;
  }

  return required_size;
}

int wrap_removexattr(const char *path, const char *name)
{
  S3_LOG(LOG_DEBUG, "removexattr", "path: %s, name: %s\n", path, name);
  ASSERT_LEADING_SLASH(path);

  return g_fs->remove_attr(path + 1, name);
}

int print_version()
{
  printf("%s, version %s\n", s3::APP_NAME, s3::APP_VERSION);

  return 0;
}

int print_usage(const char *arg0)
{
  fprintf(stderr, 
    "Usage: %s [OPTION] MOUNT_POINT\n"
    "\n"
    "Options:\n"
    "  -c, --config=FILE    use FILE as the configuration file\n"
    "  -h, --help           print this help message and exit\n"
    "  -o OPT...            pass OPT (comma-separated) to FUSE\n"
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

  if (key == FUSE_OPT_KEY_NONOPT)
    opt->have_mountpoint = true; // assume that the mountpoint is the only non-option

  return 1;
}

int main(int argc, char **argv)
{
  int r;
  fuse_operations ops;
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  options opts;

  opts.verbosity = LOG_ERR;
  opts.arg0 = argv[0];
  opts.have_mountpoint = false;

  fuse_opt_parse(&args, &opts, NULL, process_argument);

  if (!opts.have_mountpoint) {
    print_usage(opts.arg0);
    exit(1);
  }

  memset(&ops, 0, sizeof(ops));

  ops.access = wrap_access;
  ops.chmod = wrap_chmod;
  ops.chown = wrap_chown;
  ops.create = wrap_create;
  ops.getattr = wrap_getattr;
  ops.getxattr = wrap_getxattr;
  ops.flush = wrap_flush;
  ops.listxattr = wrap_listxattr;
  ops.mkdir = wrap_mkdir;
  ops.open = wrap_open;
  ops.read = wrap_read;
  ops.readdir = wrap_readdir;
  ops.readlink = wrap_readlink;
  ops.release = wrap_release;
  ops.removexattr = wrap_removexattr;
  ops.rename = wrap_rename;
  ops.rmdir = wrap_rmdir;
  ops.setxattr = wrap_setxattr;
  ops.symlink = wrap_symlink;
  ops.truncate = wrap_truncate;
  ops.unlink = wrap_unlink;
  ops.utimens = wrap_utimens;
  ops.write = wrap_write;

  s3::logger::init(opts.verbosity);

  r = s3::config::init(opts.config);

  if (r)
    return r;

  g_fs = new s3::fs();

  r = fuse_main(args.argc, args.argv, &ops, NULL);

  delete g_fs;
  return r;
}
