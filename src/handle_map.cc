#include "handle_map.h"

using namespace boost;
using namespace std;

using namespace s3;

handle_map::handle_map()
  : _next_handle(1)
{
}

file::lock handle_map::get(uint64_t handle)
{
  mutex::scoped_lock lock(_mutex);

  return _map[handle];
}

uint64_t handle_map::add_file(file *f)
{
  mutex::scoped_lock lock(_mutex);
  uint64_t handle = _next_handle++;

  _map[handle] = file::lock(f);

  return handle;
}

void handle_map::remove_file(uint64_t handle)
{
  mutex::scoped_lock lock(_mutex);

  _map.erase(handle);
}
