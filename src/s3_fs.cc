#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "s3_debug.hh"
#include "s3_fs.hh"
#include "s3_request.hh"
#include "s3_util.hh"

using namespace pugi;
using namespace std;

using namespace s3;

// TODO: try/catch everywhere!
// TODO: check error codes after run()

namespace
{
  const int BLOCK_SIZE = 512;
  const int STATS_CACHE_EXPIRY_IN_S = 120;

  int g_default_uid = 1000;
  int g_default_gid = 1000;
  int g_default_mode = 0755;

  void get_object_metadata(request *req, mode_t *mode, uid_t *uid, gid_t *gid)
  {
    const string &mode_str = req->get_response_header("x-amz-meta-s3fuse-mode");
    const string & uid_str = req->get_response_header("x-amz-meta-s3fuse-uid" );
    const string & gid_str = req->get_response_header("x-amz-meta-s3fuse-gid" );

    *mode = (mode_str.empty() ? g_default_mode : strtol(mode_str.c_str(), NULL, 0));
    * uid = ( uid_str.empty() ? g_default_uid  : strtol( uid_str.c_str(), NULL, 0));
    * gid = ( gid_str.empty() ? g_default_gid  : strtol( gid_str.c_str(), NULL, 0));
  }

  void set_object_metadata(request *req, mode_t mode, uid_t uid, gid_t gid)
  {
    char buf[16];

    snprintf(buf, 16, "%#o", mode);
    req->set_header("x-amz-meta-s3fuse-mode", buf);

    snprintf(buf, 16, "%i", uid);
    req->set_header("x-amz-meta-s3fuse-uid", buf);

    snprintf(buf, 16, "%i", gid);
    req->set_header("x-amz-meta-s3fuse-gid", buf);
  }
}

fs::fs(const string &bucket)
  : _bucket(string("/") + util::url_encode(bucket)),
    _prefix_query("/ListBucketResult/CommonPrefixes/Prefix"),
    _key_query("/ListBucketResult/Contents")
{
}

fs::~fs()
{
}

bool fs::get_cached_stats(const std::string &path, struct stat *s)
{
  file_stats &fs = _stats_map[path];

  // TODO: lock?

  if (fs.expiry == 0)
    return false;

  if (fs.expiry < time(NULL)) {
    S3_DEBUG("fs::get_cached_stats", "[%s] in cache, but expired (%i vs. %i)\n", path.c_str(), fs.expiry, time(NULL));
    return false;
  }

  memcpy(s, &fs.stats, sizeof(*s));
  return true;
}

void fs::update_stats_cache(const std::string &path, const struct stat *s)
{
  file_stats &fs = _stats_map[path];

  fs.expiry = time(NULL) + STATS_CACHE_EXPIRY_IN_S;
  memcpy(&fs.stats, s, sizeof(*s));
}

int fs::get_stats(const string &path, struct stat *s)
{
  request req(HTTP_HEAD);
  bool is_directory = true;

  memset(s, 0, sizeof(*s));

  if (path[path.size() - 1] == '/')
    return -EINVAL;

  if (get_cached_stats(path, s))
    return 0;

  // see if the path is a directory (trailing /) first
  req.set_url(_bucket + "/" + util::url_encode(path) + "/", "");
  req.run();

  // it's not a directory
  if (req.get_response_code() != 200) {
    is_directory = false;
    req.set_url(_bucket + "/" + util::url_encode(path), "");
    req.run();

    if (req.get_response_code() != 200)
      return -ENOENT;
  }

  get_object_metadata(&req, &s->st_mode, &s->st_uid, &s->st_gid);
  const string &length = req.get_response_header("Content-Length");

  // TODO: support symlinks?
  s->st_mode |= (is_directory ? S_IFDIR : S_IFREG);
  s->st_size = strtol(length.c_str(), NULL, 0);
  s->st_nlink = 1; // laziness (see FUSE FAQ re. find)
  s->st_mtime =  req.get_last_modified();

  if (!is_directory)
    s->st_blocks = (s->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  update_stats_cache(path, s);

  return 0;
}

int fs::read_directory(const std::string &_path, fuse_fill_dir_t filler, void *buf)
{
  request req(HTTP_GET);
  size_t path_len;
  string marker = "";
  bool truncated = true;
  struct stat s;
  string path;

  // TODO: use worker threads to call get_stats in parallel!
  // TODO: openssl locking?

  if (_path[_path.size() - 1] == '/')
    return -EINVAL;

  if (!_path.empty())
    path = _path + "/";

  path_len = path.size();

  while (truncated) {
    xml_document doc;
    xml_parse_result res;
    xpath_node_set prefixes, keys;
    const char *is_truncated;

    req.set_url(_bucket, string("delimiter=/&prefix=") + util::url_encode(path) + "&marker=" + marker);
    req.run();

    res = doc.load_buffer(req.get_response_data().data(), req.get_response_data().size());

    truncated = (strcmp(doc.document_element().child_value("IsTruncated"), "true") == 0);
    prefixes = _prefix_query.evaluate_node_set(doc);
    keys = _key_query.evaluate_node_set(doc);

    if (truncated)
      marker = doc.document_element().child_value("NextMarker");

    for (xpath_node_set::const_iterator itor = prefixes.begin(); itor != prefixes.end(); ++itor) {
      const char *value = itor->node().child_value() + path_len;
      string v;

      v.assign(value, strlen(value) - 1);

      S3_DEBUG("fs::read_directory", "found common prefix [%s]\n", v.c_str());

      filler(buf, v.c_str(), get_cached_stats(v, &s) ? &s : NULL, 0);
    }

    for (xpath_node_set::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *value = itor->node().child_value("Key");

      if (strcmp(path.data(), value) != 0) {
        value += path_len;
        S3_DEBUG("fs::read_directory", "found key [%s]\n", value);

        filler(buf, value, get_cached_stats(value, &s) ? &s : NULL, 0);
      }
    }
  }

  return 0;
}

int fs::create_object(const std::string &path, mode_t mode)
{
  request req(HTTP_PUT);
  string url;

  if (path[path.size() - 1] == '/')
    return -EINVAL;

  url = _bucket + "/" + util::url_encode(path);

  if (S_ISDIR(mode))
    url += "/";

  req.set_url(url, "");
  req.set_header("Content-Type", "binary/octet-stream");

  if (mode & ~S_IFMT == 0) {
    S3_DEBUG("fs::create_object", "no mode specified, using default.\n");
    mode |= g_default_mode;
  }

  // TODO: use parent?
  set_object_metadata(&req, mode, g_default_uid, g_default_gid);

  req.run();

  return 0;
}
