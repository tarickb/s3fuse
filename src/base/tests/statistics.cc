#include <boost/lexical_cast.hpp>

#include "base/statistics.h"

using boost::lexical_cast;
using std::cout;
using std::string;

using s3::base::statistics;

int main(int argc, char **argv)
{
  statistics::init();

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++)
      statistics::post("object", i, "%i", j);
  }

  statistics::write();

  return 0;
}
