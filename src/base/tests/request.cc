#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <stdexcept>

#include "base/config.h"
#include "base/request.h"

namespace s3 {
namespace base {
namespace tests {

namespace {
constexpr int REQUEST_TIMEOUT = 2;
}

TEST(Request, BadUrl) {
  auto r = RequestFactory::NewNoHook();

  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("some:bad:url");

  ASSERT_THROW(r->Run(REQUEST_TIMEOUT), std::runtime_error);
}

TEST(Request, MissingPage) {
  auto r = RequestFactory::NewNoHook();

  base::Config::set_max_transfer_retries(0);
  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("https://httpstat.us/404");
  ASSERT_NO_THROW(r->Run(REQUEST_TIMEOUT));

  ASSERT_EQ(s3::base::HTTP_SC_NOT_FOUND, r->response_code());
}

TEST(Request, ValidPageHttp) {
  auto r = RequestFactory::NewNoHook();

  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("http://www.google.com/");
  ASSERT_NO_THROW(r->Run(REQUEST_TIMEOUT));

  ASSERT_EQ(s3::base::HTTP_SC_OK, r->response_code());
  ASSERT_FALSE(r->GetOutputAsString().empty());
}

TEST(Request, ValidPageHttps) {
  auto r = RequestFactory::NewNoHook();

  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("https://www.google.com/");
  ASSERT_NO_THROW(r->Run(REQUEST_TIMEOUT));

  ASSERT_EQ(s3::base::HTTP_SC_OK, r->response_code());
  ASSERT_FALSE(r->GetOutputAsString().empty());
}

TEST(Request, Timeout) {
  auto r = RequestFactory::NewNoHook();

  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("https://httpstat.us/200?sleep=30000" /* in ms */);

  std::string caught;
  try {
    r->Run(REQUEST_TIMEOUT);
  } catch (const std::exception &e) {
    caught = e.what();
  }
  EXPECT_THAT(caught, ::testing::HasSubstr("timed out"));
}

TEST(Request, InvalidHostname) {
  auto r = RequestFactory::NewNoHook();

  r->Init(s3::base::HttpMethod::GET);
  r->SetUrl("https://abcdef.ghijkl.mnopqr.st/");

  std::string caught;
  try {
    r->Run(REQUEST_TIMEOUT);
  } catch (const std::exception &e) {
    caught = e.what();
  }
  EXPECT_THAT(caught, ::testing::HasSubstr("resolve"));
}

}  // namespace tests
}  // namespace base
}  // namespace s3
