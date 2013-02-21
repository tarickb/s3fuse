#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/detail/atomic_count.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "crypto/hash.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "services/gs_file_transfer.h"
#include "threads/pool.h"

using boost::lexical_cast;
using boost::scoped_ptr;
using boost::detail::atomic_count;
using std::ostream;
using std::string;
using std::vector;

using s3::base::char_vector;
using s3::base::char_vector_ptr;
using s3::base::config;
using s3::base::request;
using s3::base::statistics;
using s3::base::xml;
using s3::crypto::hash;
using s3::crypto::hex_with_quotes;
using s3::crypto::md5;
using s3::services::gs_file_transfer;
using s3::threads::pool;

namespace
{
  const size_t UPLOAD_CHUNK_SIZE = 256 * 1024;

  atomic_count s_uploads_multi_chunks_failed(0);

  void statistics_writer(ostream *o)
  {
    *o <<
      "gs_file_transfer multi-part uploads:\n"
      "  chunks failed: " << s_uploads_multi_chunks_failed << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

gs_file_transfer::gs_file_transfer()
{
  _upload_chunk_size = 
    (config::get_upload_chunk_size() == -1)
      ? UPLOAD_CHUNK_SIZE
      : config::get_upload_chunk_size();
}

size_t gs_file_transfer::get_upload_chunk_size()
{
  return _upload_chunk_size;
}

int gs_file_transfer::upload_multi(const string &url, size_t size, const read_chunk_fn &on_read, string *returned_etag)
{
  return -ENOTSUP;
}
