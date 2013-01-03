#include <stdexcept>
#include <gtest/gtest.h>

#include "base/request.h"

using std::runtime_error;

using s3::base::request;

TEST(request, bad_url)
{
  request r;

  r.init(s3::base::HTTP_GET);
  r.set_url("some:bad:url");

  ASSERT_THROW(r.run(), runtime_error);
}

TEST(request, missing_page)
{
  request r;

  r.init(s3::base::HTTP_GET);
  r.set_url("http://www.google.com/this_shouldnt_exist");
  r.run();

  ASSERT_EQ(s3::base::HTTP_SC_NOT_FOUND, r.get_response_code());
}

TEST(request, valid_page)
{
  request r;

  r.init(s3::base::HTTP_GET);
  r.set_url("http://www.google.com/");
  r.run();

  ASSERT_EQ(s3::base::HTTP_SC_OK, r.get_response_code());
  ASSERT_FALSE(r.get_output_string().empty());
}
