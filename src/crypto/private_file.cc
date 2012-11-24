#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>

#include "crypto/private_file.h"

using std::ifstream;
using std::ios;
using std::ofstream;
using std::runtime_error;
using std::string;

using s3::crypto::private_file;

void private_file::open(const string &file, ofstream *f)
{
  f->open(file.c_str(), ios::out | ios::trunc);

  if (!f->good())
    throw runtime_error("unable to open/create private file.");

  if (chmod(file.c_str(), S_IRUSR | S_IWUSR))
    throw runtime_error("failed to set permissions on private file.");
}

void private_file::open(const string &file, ifstream *f)
{
  struct stat s;

  f->open(file.c_str(), ios::in);

  if (!f->good())
    throw runtime_error("unable to open private file.");

  if (stat(file.c_str(), &s))
    throw runtime_error("unable to stat private file.");

  if ((s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRUSR | S_IWUSR))
    throw runtime_error("private file must be readable/writeable only by owner.");
}
