#include <iostream>

#include "base/timer.h"

using std::cout;
using std::endl;

using s3::base::timer;

int main(int argc, char **argv)
{
  cout << "current time in seconds: " << timer::get_current_time() << endl;
  cout << "http time: " << timer::get_http_time() << endl;

  return 0;
}
