#include "config.h"
#include "directory.h"
#include "logger.h"
#include "request.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const char *IS_TRUNCATED_XPATH = "/s3:ListBucketResult/s3:IsTruncated";
  const char *         KEY_XPATH = "/s3:ListBucketResult/s3:Contents/s3:Key";
  const char * NEXT_MARKER_XPATH = "/s3:ListBucketResult/s3:NextMarker";
  const char *      PREFIX_XPATH = "/s3:ListBucketResult/s3:CommonPrefixes/s3:Prefix";

  int check_if_truncated(const xml::document &doc, bool *truncated)
  {
    int r;
    string temp;

    r = xml::find(doc, IS_TRUNCATED_XPATH, &temp);

    if (r)
      return r;

    *truncated = (temp == "true");
    return 0;
  }
}

directory::directory(const string &path)
  : object(path)
{
  init();
}

object_type directory::get_type()
{
  return OT_DIRECTORY;
}

mode_t directory::get_mode()
{
  return S_IFDIR;
}

int directory::fill(const request::ptr &req, const filler_fn &filler)
{
  mutex::scoped_lock lock(get_mutex());
  cache_list_ptr cache;
  string path = get_path();
  string marker = "";
  size_t path_len;
  bool truncated = true;

  // make local copy and unlock mutex
  cache = _cache;
  lock.unlock();

  if (cache) {
    for (cache_list::const_iterator itor = cache->begin(); itor != cache->end(); ++itor)
      filler(*itor);

    return 0;
  }

  if (config::get_cache_directories())
    cache.reset(new cache_list());

  // TODO: replace with build_url() or something
  if (!path.empty())
    path += '/';

  path_len = path.size();

  req->init(HTTP_GET);

  while (truncated) {
    int r;
    xml::document doc;
    xml::element_list prefixes, keys;

    req->set_url(object::get_bucket_url(), string("delimiter=/&prefix=") + util::url_encode(path) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != HTTP_SC_OK)
      return -EIO;

    doc = xml::parse(req->get_response_data());

    if (!doc) {
      S3_LOG(LOG_WARNING, "directory::fill", "failed to parse response.\n");
      return -EIO;
    }

    if ((r = check_if_truncated(doc, &truncated)))
      return r;

    if (truncated && (r = xml::find(doc, NEXT_MARKER_XPATH, &marker)))
      return r;

    if ((r = xml::find(doc, PREFIX_XPATH, &prefixes)))
      return r;

    if ((r = xml::find(doc, KEY_XPATH, &keys)))
      return r;

    for (xml::element_list::const_iterator itor = prefixes.begin(); itor != prefixes.end(); ++itor) {
      const char *full_path_cs = itor->c_str();
      const char *relative_path_cs = full_path_cs + path_len;
      string full_path, relative_path;

      // strip trailing slash
      full_path.assign(full_path_cs, strlen(full_path_cs) - 1);
      relative_path.assign(relative_path_cs, strlen(relative_path_cs) - 1);

      // TODO: fix this
      // _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_DIR));
      filler(relative_path);

      if (cache)
        cache->push_back(relative_path);
    }

    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      const char *full_path_cs = itor->c_str();

      if (strcmp(path.c_str(), full_path_cs) != 0) {
        string relative_path(full_path_cs + path_len);
        string full_path(full_path_cs);

        // TODO: fix this
        // _tp_bg->call_async(bind(&fs::__prefill_stats, this, _1, full_path, HINT_IS_FILE));
        filler(relative_path);

        if (cache)
          cache->push_back(relative_path);
      }
    }
  }

  if (cache) {
    lock.lock();
    _cache = cache;
  }

  return 0;
}
 
bool directory::is_empty(const request::ptr &req)
{
  const std::string &path = get_path();
  xml::document doc;
  xml::element_list keys;

  // root directory may be empty, but we won't allow its removal
  if (path.empty())
    return false;

  req->init(HTTP_GET);

  // set max-keys to two because GET will always return the path we request
  // note the trailing slash on path
  req->set_url(object::get_bucket_url(), string("prefix=") + util::url_encode(path) + "/&max-keys=2");
  req->run();

  // if the request fails, assume the directory's not empty
  if (req->get_response_code() != HTTP_SC_OK)
    return false;

  doc = xml::parse(req->get_response_data());

  if (!doc) {
    S3_LOG(LOG_WARNING, "directory::is_empty", "failed to parse response.\n");
    return false;
  }

  if (xml::find(doc, KEY_XPATH, &keys))
    return false;

  return (keys.size() == 1);
}
