CONFIG(int, default_uid, 0);
CONFIG(int, default_gid, 0);
CONFIG(int, default_mode, 0755);
CONFIG(int, mountpoint_mode, 0755);

CONFIG(int, cache_expiry_in_s, 3 * 60);

CONFIG(std::string, default_content_type, "binary/octet-stream");
CONFIG(size_t, download_chunk_size, 128 * 1024);
CONFIG(size_t, upload_chunk_size, 5 * 1024 * 1024);
CONFIG(int, max_transfer_retries, 5);

CONFIG(std::string, url_prefix, "https://s3.amazonaws.com");
CONFIG(bool, verbose_requests, false);

CONFIG_REQUIRED(std::string, aws_key, "");
CONFIG_REQUIRED(std::string, aws_secret, "");
CONFIG_REQUIRED(std::string, bucket_name, "");
