// this file is used to generate config.h, config.cc, and s3fuse.conf
//
// format is:
// CONFIG(type, name, default-value); or CONFIG_REQUIRED(type, name, default-value);
//
// notes:
//   - a semicolon must follow CONFIG() or CONFIG_REQUIRED().
//   - a description can follow.  if it does, it must follow the semicolon and be preceded by " // ".
//   - make-config.sh does not preprocess this file, so there's not much room for variation in formatting

CONFIG(int, default_uid, UID_MAX); // UID for files in S3 that don't have a UID set (defaults to process effective UID)
CONFIG(int, default_gid, GID_MAX); // GID for files in S3 that don't have a GID set (defaults to process effective GID)
CONFIG(int, default_mode, 0755); // mode for files in S3 that don't have a mode set

CONFIG(int, cache_expiry_in_s, 3 * 60); // time in seconds before objects in stats cache expire
CONFIG(bool, cache_directories, false); // cache directory listings ("true"/"yes" or "false"/"no")
CONFIG(int, max_objects_in_cache, 1000); // maximum number of objects to hold in cache

CONFIG(std::string, default_content_type, "binary/octet-stream"); // MIME type for newly-created objects
CONFIG(size_t, download_chunk_size, 128 * 1024); // download chunk size, in bytes
CONFIG(size_t, upload_chunk_size, 5 * 1024 * 1024); // upload chunk size, in bytes (5 MB is the S3 minimum)
CONFIG(int, max_transfer_retries, 5); // maximum number of times a chunk transfer will be retried before failing
CONFIG(int, transfer_timeout_in_s, 5 * 60); // transfer timeout in seconds, should be long enough to transfer download_chunk_size/upload_chunk_size

CONFIG(bool, verbose_requests, false); // set CURLOPT_VERBOSE (enable verbosity in libcurl)
CONFIG(int, request_timeout_in_s, 30); // request timeout in seconds (for all HTTP requests besides transfers)

CONFIG(std::string, aws_service_endpoint, "s3.amazonaws.com"); // service endpoint for Amazon AWS (change to "s3-eu-west-1.amazonaws.com" for EU buckets)
CONFIG(bool, aws_use_ssl, true); // set to false to disable SSL

CONFIG(bool, use_encryption, false); // if set, encryption/decryption support will be enabled
CONFIG(bool, encrypt_new_files, true); // if set, all newly-created files will be encrypted
CONFIG(std::string, volume_key_file, ""); // name of file containing hex-encoded volume key. if blank, read from stdin.

CONFIG(bool, allow_glacier_restores, false); // enable glacier object restores

CONFIG(int, max_inconsistent_state_retries, 10); // number of times to retry an operation if an inconsistent state is encountered (must be >= 2)

CONFIG_REQUIRED(std::string, service, ""); // service -- must be "aws" or "google-storage"
CONFIG_REQUIRED(std::string, auth_data, ""); // service-specific authentication details (see documentation)
CONFIG_REQUIRED(std::string, bucket_name, ""); // bucket name

CONFIG_CONSTRAINT(CONFIG_KEY(max_objects_in_cache) > 0, "max_objects_in_cache must be greater than zero");
CONFIG_CONSTRAINT(CONFIG_KEY(max_transfer_retries) > 0, "max_transfer_retries must be greater than zero");
CONFIG_CONSTRAINT(CONFIG_KEY(max_inconsistent_state_retries) >= 2, "max_inconsistent_state_retries must be greater than or equal to 2");
