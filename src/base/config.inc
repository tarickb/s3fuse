// this file is used to generate config.h, config.cc, s3fuse.conf, and 
// s3fuse.conf.5
//
// macros:
//
// CONFIG(type, name, default-value, description);
//   declare a configuration option.
//
// CONFIG_REQUIRED(type, name, default-value, description);
//   declare a required configuration option.
//
// CONFIG_CONSTRAINT(test, error);
//   describe a constraint. "test" must evaluate to a boolean and can use
//   CONFIG_KEY(x) to get the value of configuration key "x". the message in
//   "error" is printed if "test" is false.
//
// notes:
//   - all macro invocations must be followed by a semicolon.
//   - this file won't always be run through a preprocessor, so there's not
//     much room for variation in formatting.

// basics
CONFIG_REQUIRED(std::string, service, "", "specifies the service to use -- must be 'aws' or 'google-storage'");
CONFIG_REQUIRED(std::string, auth_data, "", "service authentication data");
CONFIG_REQUIRED(std::string, bucket_name, "", "bucket name");

// default uid/gid/mode
CONFIG(int, default_uid, UID_MAX, "UID for files in S3 that don't have a UID set (defaults to process effective UID)");
CONFIG(int, default_gid, GID_MAX, "GID for files in S3 that don't have a GID set (defaults to process effective GID)");
CONFIG(int, default_mode, 0755, "mode for files in S3 that don't have a mode set");

// cache options
CONFIG(int, cache_expiry_in_s, 3 * 60, "time in seconds before objects in stats cache expire");
CONFIG(bool, cache_directories, false, "cache directory listings ('true'/'yes' or 'false'/'no')");
CONFIG(int, max_objects_in_cache, 1000, "maximum number of objects to hold in cache");
CONFIG_CONSTRAINT(CONFIG_KEY(max_objects_in_cache) > 0, "max_objects_in_cache must be greater than zero");

// MIME
CONFIG(std::string, default_content_type, "binary/octet-stream", "MIME type for newly-created objects");

// uploads/downloads
CONFIG(size_t, download_chunk_size, 128 * 1024, "download chunk size in bytes");
CONFIG(size_t, upload_chunk_size, 5 * 1024 * 1024, "upload chunk size in bytes (5 MB is the S3 minimum)");
CONFIG(int, max_transfer_retries, 5, "maximum number of times a chunk transfer will be retried before failing");
CONFIG(int, transfer_timeout_in_s, 5 * 60, "transfer timeout in seconds; should be long enough to transfer download_chunk_size/upload_chunk_size");
CONFIG_CONSTRAINT(CONFIG_KEY(max_transfer_retries) > 0, "max_transfer_retries must be greater than zero");

// debug
CONFIG(bool, verbose_requests, false, "set CURLOPT_VERBOSE (enable verbosity in libcurl)");

// timeouts
CONFIG(int, request_timeout_in_s, 30, "request timeout in seconds (for all HTTP requests besides transfers)");
CONFIG(int, max_inconsistent_state_retries, 10, "number of times to retry an operation if an inconsistent state is encountered (must be >= 2)");
CONFIG_CONSTRAINT(CONFIG_KEY(max_inconsistent_state_retries) >= 2, "max_inconsistent_state_retries must be greater than or equal to 2");

// AWS S3
CONFIG(std::string, aws_service_endpoint, "s3.amazonaws.com", "service endpoint for Amazon AWS (change to 's3-eu-west-1.amazonaws.com' for EU buckets)");
CONFIG(bool, aws_use_ssl, true, "set to false to disable SSL");

// encryption
CONFIG(bool, use_encryption, false, "encryption/decryption support will be enabled if set to 'true'/'yes'");
CONFIG(bool, encrypt_new_files, true, "all newly-created files will be encrypted if set to 'true'/'yes'");
CONFIG(std::string, volume_key_file, "", "name of file containing hex-encoded volume key. will prompt for password if blank.");

// glacier
CONFIG(bool, allow_glacier_restores, false, "enable glacier object restores");
