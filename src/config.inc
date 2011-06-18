// format is:
// CONFIG(type, name, default-value)

CONFIG(int, default_uid, UID_MAX);
CONFIG(int, default_gid, GID_MAX);
CONFIG(int, default_mode, 0755);

CONFIG(int, cache_expiry_in_s, 3 * 60);

CONFIG(std::string, default_content_type, "binary/octet-stream");
CONFIG(size_t, download_chunk_size, 128 * 1024);
CONFIG(size_t, upload_chunk_size, 5 * 1024 * 1024);
CONFIG(int, max_transfer_retries, 5);

CONFIG(std::string, url_prefix, "https://s3.amazonaws.com");
CONFIG(bool, verbose_requests, false);
CONFIG(int, request_timeout_in_s, 5 * 60); // should be long enough to transfer upload_chunk_size

CONFIG_REQUIRED(std::string, aws_key, "");
CONFIG_REQUIRED(std::string, aws_secret, "");
CONFIG_REQUIRED(std::string, bucket_name, "");
