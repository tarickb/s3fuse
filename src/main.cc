#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "config.h"
#include "fs.h"
#include "logger.h"
#include "version.h"

using namespace boost;
using namespace std;

#define ASSERT_LEADING_SLASH(str) do { if ((str)[0] != '/') return -EINVAL; } while (0)

namespace
{
  struct options
  {
    const char *arg0;
    string config;
    string mountpoint;
    int verbosity;
  };

  int try_catch(boost::function0<int> fn)
  {
    try {
      return fn();

    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "try_catch", "caught exception: %s\n", e.what());

    } catch (...) {
      S3_LOG(LOG_WARNING, "try_catch", "caught unknown exception\n");
    }

    return -ECANCELED;
  }

  s3::fs *s_fs;
  int s_mountpoint_mode;
}

int wrap_getattr(const char *path, struct stat *s)
{
  ASSERT_LEADING_SLASH(path);

  memset(s, 0, sizeof(*s));

  if (strcmp(path, "/") == 0) {
    s->st_mode = s_mountpoint_mode;
    s->st_nlink = 1; // because calculating nlink is hard! (see FUSE FAQ)

    return 0;
  }

  return try_catch(bind(&s3::fs::get_stats, s_fs, path + 1, s));
}

int wrap_chown(const char *path, uid_t uid, gid_t gid)
{
  S3_LOG(LOG_DEBUG, "chown", "path: %s, user: %i, group: %i\n", path, uid, gid);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::change_owner, s_fs, path + 1, uid, gid));
}

int wrap_chmod(const char *path, mode_t mode)
{
  S3_LOG(LOG_DEBUG, "chmod", "path: %s, mode: %i\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::change_mode, s_fs, path + 1, mode));
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "readdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::read_directory, s_fs, path + 1, filler, buf));
}

int wrap_mkdir(const char *path, mode_t mode)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "mkdir", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::create_directory, s_fs, path + 1, mode, ctx->uid, ctx->gid));
}

int wrap_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
  int r;
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "create", "path: %s, mode: %#o\n", path, mode);
  ASSERT_LEADING_SLASH(path);

  r = try_catch(bind(&s3::fs::create_file, s_fs, path + 1, mode, ctx->uid, ctx->gid));

  if (r)
    return r;

  return try_catch(bind(&s3::fs::open, s_fs, path + 1, &file_info->fh));
}

int wrap_open(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "open", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::open, s_fs, path + 1, &file_info->fh));
}

int wrap_read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::read, s_fs, file_info->fh, buffer, size, offset));
}

int wrap_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info)
{
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::write, s_fs, file_info->fh, buffer, size, offset));
}

int wrap_flush(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "flush", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::flush, s_fs, file_info->fh));
}

int wrap_release(const char *path, fuse_file_info *file_info)
{
  S3_LOG(LOG_DEBUG, "release", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::release, s_fs, file_info->fh));
}

int wrap_truncate(const char *path, off_t offset)
{
  S3_LOG(LOG_DEBUG, "truncate", "path: %s, offset: %ji\n", path, static_cast<intmax_t>(offset));
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::truncate_by_path, s_fs, path + 1, offset));
}

int wrap_unlink(const char *path)
{
  S3_LOG(LOG_DEBUG, "unlink", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::remove_file, s_fs, path + 1));
}

int wrap_rmdir(const char *path)
{
  S3_LOG(LOG_DEBUG, "rmdir", "path: %s\n", path);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::remove_directory, s_fs, path + 1));
}

int wrap_rename(const char *from, const char *to)
{
  S3_LOG(LOG_DEBUG, "rename", "from: %s, to: %s\n", from, to);
  ASSERT_LEADING_SLASH(from);
  ASSERT_LEADING_SLASH(to);

  return try_catch(bind(&s3::fs::rename_object, s_fs, from + 1, to + 1));
}

int wrap_utimens(const char *path, const timespec times[2])
{
  S3_LOG(LOG_DEBUG, "utimens", "path: %s, time: %li\n", path, times[1].tv_sec);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::change_mtime, s_fs, path + 1, times[1].tv_sec));
}

int wrap_symlink(const char *target, const char *path)
{
  const fuse_context *ctx = fuse_get_context();

  S3_LOG(LOG_DEBUG, "symlink", "path: %s, target: %s\n", path, target);
  ASSERT_LEADING_SLASH(path);

  return try_catch(bind(&s3::fs::create_symlink, s_fs, path + 1, ctx->uid, ctx->gid, target));
}

int wrap_readlink(const char *path, char *buffer, size_t max_size)
{
  string target;
  int r;

  S3_LOG(LOG_DEBUG, "readlink", "path: %s, max_size: %zu\n", path, max_size);
  ASSERT_LEADING_SLASH(path);

  r = try_catch(bind(&s3::fs::read_symlink, s_fs, path + 1, &target));

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

  r = try_catch(bind(&s3::fs::get_attr, s_fs, path + 1, name, &attr));

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

  return try_catch(bind(&s3::fs::set_attr, s_fs, path + 1, name, value, flags));
}

int wrap_listxattr(const char *path, char *buffer, size_t size)
{
  vector<string> attrs;
  size_t required_size = 0;
  int r;

  S3_LOG(LOG_DEBUG, "listxattr", "path: %s, size: %zu\n", path, size);
  ASSERT_LEADING_SLASH(path);

  r = try_catch(bind(&s3::fs::list_attr, s_fs, path + 1, &attrs));

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

  return try_catch(bind(&s3::fs::remove_attr, s_fs, path + 1, name));
}

int print_version()
{
  printf("%s, %s, version %s\n", s3::APP_FULL_NAME, s3::APP_NAME, s3::APP_VERSION);

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
    opt->mountpoint = arg; // assume that the mountpoint is the only non-option

  return 1;
}

int pre_init(const options &opts)
{
  int r;
  struct stat mp_stat;

  if ((r = stat(opts.mountpoint.c_str(), &mp_stat)))
    return r;

  s_mountpoint_mode = S_IFDIR | mp_stat.st_mode;

  s3::logger::init(opts.verbosity);
  r = s3::config::init(opts.config);

  return r;
}

void * init(fuse_conn_info *info)
{
  s_fs = new s3::fs();

  return NULL;
}

void build_ops(fuse_operations *ops)
{
  memset(ops, 0, sizeof(*ops));

  ops->chmod = wrap_chmod;
  ops->chown = wrap_chown;
  ops->create = wrap_create;
  ops->getattr = wrap_getattr;
  ops->getxattr = wrap_getxattr;
  ops->flush = wrap_flush;
  ops->init = init;
  ops->listxattr = wrap_listxattr;
  ops->mkdir = wrap_mkdir;
  ops->open = wrap_open;
  ops->read = wrap_read;
  ops->readdir = wrap_readdir;
  ops->readlink = wrap_readlink;
  ops->release = wrap_release;
  ops->removexattr = wrap_removexattr;
  ops->rename = wrap_rename;
  ops->rmdir = wrap_rmdir;
  ops->setxattr = wrap_setxattr;
  ops->symlink = wrap_symlink;
  ops->truncate = wrap_truncate;
  ops->unlink = wrap_unlink;
  ops->utimens = wrap_utimens;
  ops->write = wrap_write;
}

int main(int argc, char **argv)
{
  int r;
  fuse_operations ops;
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  options opts;

  opts.verbosity = LOG_ERR;
  opts.arg0 = argv[0];

  fuse_opt_parse(&args, &opts, NULL, process_argument);

  if (opts.mountpoint.empty()) {
    print_usage(opts.arg0);
    exit(1);
  }

  build_ops(&ops);

  if ((r = pre_init(opts)))
    return r;

  r = fuse_main(args.argc, args.argv, &ops, NULL);
  delete s_fs;

  return r;
}
