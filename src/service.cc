#include <stdexcept>

#include "service.h"
#include "aws_service_impl.h"
#include "config.h"
#include "gs_service_impl.h"

using namespace std;

using namespace s3;

service_impl::ptr service::s_impl;

void service::init(const string &service)
{
  if (service == "aws")
    s_impl.reset(new aws_service_impl());

  else if (service == "google-storage")
    s_impl.reset(new gs_service_impl());

  else
    throw runtime_error("unrecognized service.");
}
