#include "fs/metadata.h"

using s3::fs::metadata;

const char *metadata::RESERVED_PREFIX    = "s3fuse-";

// this can't share a common prefix with RESERVED_PREFIX
const char *metadata::XATTR_PREFIX       = "s3fuse_xattr_";

const char *metadata::LAST_UPDATE_ETAG   = "s3fuse-lu-etag";
const char *metadata::MODE               = "s3fuse-mode";
const char *metadata::UID                = "s3fuse-uid";
const char *metadata::GID                = "s3fuse-gid";
const char *metadata::LAST_MODIFIED_TIME = "s3fuse-mtime";

const char *metadata::SHA256             = "s3fuse-sha256";
const char *metadata::ENC_IV             = "s3fuse-e-iv";
const char *metadata::ENC_METADATA       = "s3fuse-e-meta";
