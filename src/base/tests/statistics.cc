#include <sstream>
#include <gtest/gtest.h>

#include "base/statistics.h"

using boost::shared_ptr;
using std::endl;
using std::ostream;
using std::ostringstream;
using std::string;

using s3::base::statistics;

TEST(statistics, write_100)
{
  shared_ptr<ostringstream> out(new ostringstream());
  string exp;

  statistics::init(out);

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++)
      statistics::write("object", i, "%i", j);
  }

  statistics::flush();

  exp =
    "object_0: 0\n"
    "object_0: 1\n"
    "object_0: 2\n"
    "object_0: 3\n"
    "object_0: 4\n"
    "object_0: 5\n"
    "object_0: 6\n"
    "object_0: 7\n"
    "object_0: 8\n"
    "object_0: 9\n"
    "object_1: 0\n"
    "object_1: 1\n"
    "object_1: 2\n"
    "object_1: 3\n"
    "object_1: 4\n"
    "object_1: 5\n"
    "object_1: 6\n"
    "object_1: 7\n"
    "object_1: 8\n"
    "object_1: 9\n"
    "object_2: 0\n"
    "object_2: 1\n"
    "object_2: 2\n"
    "object_2: 3\n"
    "object_2: 4\n"
    "object_2: 5\n"
    "object_2: 6\n"
    "object_2: 7\n"
    "object_2: 8\n"
    "object_2: 9\n"
    "object_3: 0\n"
    "object_3: 1\n"
    "object_3: 2\n"
    "object_3: 3\n"
    "object_3: 4\n"
    "object_3: 5\n"
    "object_3: 6\n"
    "object_3: 7\n"
    "object_3: 8\n"
    "object_3: 9\n"
    "object_4: 0\n"
    "object_4: 1\n"
    "object_4: 2\n"
    "object_4: 3\n"
    "object_4: 4\n"
    "object_4: 5\n"
    "object_4: 6\n"
    "object_4: 7\n"
    "object_4: 8\n"
    "object_4: 9\n"
    "object_5: 0\n"
    "object_5: 1\n"
    "object_5: 2\n"
    "object_5: 3\n"
    "object_5: 4\n"
    "object_5: 5\n"
    "object_5: 6\n"
    "object_5: 7\n"
    "object_5: 8\n"
    "object_5: 9\n"
    "object_6: 0\n"
    "object_6: 1\n"
    "object_6: 2\n"
    "object_6: 3\n"
    "object_6: 4\n"
    "object_6: 5\n"
    "object_6: 6\n"
    "object_6: 7\n"
    "object_6: 8\n"
    "object_6: 9\n"
    "object_7: 0\n"
    "object_7: 1\n"
    "object_7: 2\n"
    "object_7: 3\n"
    "object_7: 4\n"
    "object_7: 5\n"
    "object_7: 6\n"
    "object_7: 7\n"
    "object_7: 8\n"
    "object_7: 9\n"
    "object_8: 0\n"
    "object_8: 1\n"
    "object_8: 2\n"
    "object_8: 3\n"
    "object_8: 4\n"
    "object_8: 5\n"
    "object_8: 6\n"
    "object_8: 7\n"
    "object_8: 8\n"
    "object_8: 9\n"
    "object_9: 0\n"
    "object_9: 1\n"
    "object_9: 2\n"
    "object_9: 3\n"
    "object_9: 4\n"
    "object_9: 5\n"
    "object_9: 6\n"
    "object_9: 7\n"
    "object_9: 8\n"
    "object_9: 9\n";

  EXPECT_EQ(exp, out->str());
}
