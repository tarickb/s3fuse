#include <sys/stat.h>

#include <stdexcept>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "fs/local_file.h"
#include "fs/local_file_store.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::base::config;
using s3::fs::local_file;
using s3::fs::local_file_store;

namespace
{
  const string TEMP_FILE_TEMPLATE = "s3fuse.local-XXXXXX";

  vector<char> s_temp_name_template;
}

void local_file::init()
{
  string temp = config::get_local_store_path();

  if (temp.empty() || temp[temp.size() - 1] != '/')
    temp += "/";
 
  temp += TEMP_FILE_TEMPLATE;

  s_temp_name_template.resize(temp.size() + 1);
  memcpy(&s_temp_name_template[0], temp.c_str(), temp.size() + 1);
}

local_file::local_file(size_t size)
  : _size(size)
{
  vector<char> temp_name = s_temp_name_template;

  _fd = mkstemp(&temp_name[0]);
  unlink(&temp_name[0]);

  S3_LOG(LOG_DEBUG, "local_file::open", "opening local file in [%s].\n", &temp_name[0]);

  if (_fd == -1)
    throw runtime_error("failed to open file in local store");

  if (ftruncate(_fd, _size) != 0)
    throw runtime_error("failed to truncate local store file");

  local_file_store::increment(_size);
}

local_file::~local_file()
{
  close(_fd);

  local_file_store::decrement(_size);
}

size_t local_file::get_size()
{
  struct stat s;

  if (fstat(_fd, &s))
    throw runtime_error("failed to stat local file");

  return s.st_size;
}

void local_file::refresh_store_size()
{
  local_file_store::decrement(_size);

  _size = get_size();
  
  local_file_store::increment(_size);
}
