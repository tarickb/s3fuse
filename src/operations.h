/*
 * operations.h
 * -------------------------------------------------------------------------
 * Filesystem operation declarations.
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

#ifndef S3_OPERATIONS_H
#define S3_OPERATIONS_H

#define FUSE_USE_VERSION 26

#include <fuse.h>

#include <string>

namespace s3
{
  class operations
  {
  public:
    static void init(const std::string &mountpoint);
    static void build_fuse_operations(fuse_operations *ops);

  private:
    static int chmod(const char *path, mode_t mode);
    static int chown(const char *path, uid_t uid, gid_t gid);
    static int create(const char *path, mode_t mode, fuse_file_info *file_info);
    static int flush(const char *path, fuse_file_info *file_info);
    static int ftruncate(const char *path, off_t offset, fuse_file_info *file_info);
    static int getattr(const char *path, struct stat *s);

    #ifdef __APPLE__
      static int getxattr(const char *path, const char *name, char *buffer, size_t max_size, uint32_t position);
    #else
      static int getxattr(const char *path, const char *name, char *buffer, size_t max_size);
    #endif

    static int listxattr(const char *path, char *buffer, size_t size);
    static int mkdir(const char *path, mode_t mode);
    static int mknod(const char *path, mode_t mode, dev_t dev);
    static int open(const char *path, fuse_file_info *file_info);
    static int read(const char *path, char *buffer, size_t size, off_t offset, fuse_file_info *file_info);
    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, fuse_file_info *file_info);
    static int readlink(const char *path, char *buffer, size_t max_size);
    static int release(const char *path, fuse_file_info *file_info);
    static int removexattr(const char *path, const char *name);
    static int rename(const char *from, const char *to);

    #ifdef __APPLE__
      static int setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position);
    #else
      static int setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
    #endif

    static int statfs(const char * /* ignored */, struct statvfs *s);
    static int symlink(const char *target, const char *path);
    static int unlink(const char *path);
    static int utimens(const char *path, const timespec times[2]);
    static int write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *file_info);
  };
}

#endif
