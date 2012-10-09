// this file is used to generate config.h, config.cc, and s3fuse.conf
//
// format is:
// CONFIG(type, name, default-value); or CONFIG_REQUIRED(type, name, default-value);
//
// notes:
//   - a semicolon must follow CONFIG() or CONFIG_REQUIRED().
//   - a description can follow.  if it does, it must follow the semicolon and be preceded by " // ".
//   - build-config.sh does not preprocess this file, so there's not much room for variation in formatting

CONFIG(int, default_uid, UID_MAX); // UID for files in S3 that don't have a UID set (defaults to process effective UID)
CONFIG(int, default_gid, GID_MAX); // GID for files in S3 that don't have a GID set (defaults to process effective GID)
CONFIG(int, default_mode, 0755); // mode for files in S3 that don't have a mode set

CONFIG(int, cache_expiry_in_s, 3 * 60); // time in seconds before objects in stats cache expire
CONFIG(bool, cache_directories, false); // cache directory listings ("true"/"yes" or "false"/"no")

CONFIG(std::string, default_content_type, "binary/octet-stream"); // MIME type for newly-created objects
CONFIG(size_t, download_chunk_size, 128 * 1024); // download chunk size, in bytes
CONFIG(size_t, upload_chunk_size, 5 * 1024 * 1024); // upload chunk size, in bytes (5 MB is the S3 minimum)
CONFIG(int, max_transfer_retries, 5); // maximum number of times a chunk transfer will be retried before failing
CONFIG(int, transfer_timeout_in_s, 5 * 60); // transfer timeout in seconds, should be long enough to transfer download_chunk_size/upload_chunk_size

CONFIG(bool, verbose_requests, false); // set CURLOPT_VERBOSE (enable verbosity in libcurl)
CONFIG(int, request_timeout_in_s, 30); // request timeout in seconds (for all HTTP requests besides transfers)

CONFIG(std::string, aws_service_endpoint, "s3.amazonaws.com"); // service endpoint for Amazon AWS (change to "s3-eu-west-1.amazonaws.com" for EU buckets)

CONFIG(std::string, xattr_prefix, ""); // if set, prefixes all extended attributes (may have to be set to "user." for compatibility with getfattr)

CONFIG_REQUIRED(std::string, service, ""); // service -- must be "aws" or "google-storage"
CONFIG_REQUIRED(std::string, auth_data, ""); // service-specific authentication details (see documentation)
CONFIG_REQUIRED(std::string, bucket_name, ""); // bucket name
