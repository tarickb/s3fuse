#include <stdexcept>

#include "init_helper.h"
#include "base/config.h"
#include "services/aws/impl.h"
#include "services/gs/impl.h"

using std::runtime_error;
using std::string;

using s3::init_helper;
using s3::base::config;
using s3::services::impl;

impl::ptr init_helper::get_service_impl(string name)
{
  if (name.empty())
    name = config::get_service();

  if (name == "aws")
    return impl::ptr(new s3::services::aws::impl());
  else if (name == "google-storage")
    return impl::ptr(new s3::services::gs::impl());
  else
    throw runtime_error("invalid service specified.");
}
