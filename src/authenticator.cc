#include <stdexcept>

#include "authenticator.h"
#include "aws_authenticator.h"
#include "config.h"
#include "gs_authenticator.h"

using namespace std;

using namespace s3;

authenticator::ptr authenticator::create(const string &service)
{
  if (service == "aws")
    return ptr(new aws_authenticator());

  else if (service == "google-storage")
    return ptr(new gs_authenticator());

  else
    throw runtime_error("unrecognized service.");
}
