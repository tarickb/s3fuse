#ifndef S3_OPEN_FILE_HH
#define S3_OPEN_FILE_HH

/*

object::get_open_file() -> open_file::ptr

open_file_cache::get(string path)
open_file_cache::get(uint64_t context)

minimize locking:

lock on:
  - open file
  - close file
  - write/mark dirty
  - state transitions


notes:
- create an open_file object
  - contains FILE *, uint64_t context (probably just "this")
  - object::set_open_file() (can be NULL)
- open_file_cache maps uint64_t to object
  - interface for read, write, open, close, flush
  - using its own lock (per-file?)

*/

namespace s3
{
  class open_file
  {
  };
}

#endif
