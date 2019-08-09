#include <getopt.h>

#include <cstring>
#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/xml.h"
#include "services/service.h"

namespace s3 {
namespace {
constexpr char SHORT_OPTIONS[] = "c:";

constexpr option LONG_OPTIONS[] = {
    {"config-file", required_argument, nullptr, 'c'},
    {nullptr, 0, nullptr, '\0'}};

base::Request *GetRequest() {
  static auto s_request = base::RequestFactory::New();
  return s_request.get();
}

void Init(const std::string &config_file) {
  base::Logger::Init(base::Logger::Mode::STDERR, LOG_ERR);
  base::Config::Init(config_file);
  base::XmlDocument::Init();
  services::Service::Init();
}

void PrintUsage(const char *arg0) {
  const char *base_name = std::strrchr(arg0, '/');
  base_name = base_name ? base_name + 1 : arg0;
  std::cerr << "Usage: " << base_name
            << " [options] <command> [...]\n"
               "\n"
               "Where <command> is one of:\n"
               "\n"
               "  versioning [ on | off ]   Turn object versioning on or off.\n"
               "  get-versioning            Print versioning status.\n"
               "\n"
               "[options] can be:\n"
               "\n"
               "  -c, --config-file <path>  Use configuration at <path> rather "
               "than the default.\n"
               "\n"
               "See "
            << base_name << "(1) for examples and a more detailed explanation."
            << std::endl;
  exit(1);
}

void GetVersioning(const std::string &config_file) {
  constexpr char STATUS_XPATH[] = "/VersioningConfiguration/Status";

  Init(config_file);

  auto req = GetRequest();

  req->Init(s3::base::HttpMethod::GET);
  req->SetUrl(services::Service::bucket_url(), "versioning");
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "::GetVersioning", "response: %s\n",
           req->GetOutputAsString().c_str());
    throw std::runtime_error("request failed.");
  }

  const auto resp = req->GetOutputAsString();
  auto doc = base::XmlDocument::Parse(resp);
  if (!doc) {
    S3_LOG(LOG_ERR, "::GetVersioning", "malformed response: %s\n",
           resp.c_str());
    throw std::runtime_error("request failed.");
  }

  std::string status;
  doc->Find(STATUS_XPATH, &status);
  if (status.empty()) status = "Disabled";
  std::cout << "Versioning status: " << status << std::endl;
}

void SetVersioning(const std::string &config_file, bool enable) {
  Init(config_file);

  const std::string body = std::string("<VersioningConfiguration>") +
                           "<Status>" + (enable ? "Enabled" : "Suspended") +
                           "</Status>" + "</VersioningConfiguration>";

  auto req = GetRequest();

  req->Init(s3::base::HttpMethod::PUT);
  req->SetUrl(services::Service::bucket_url(), "versioning");
  req->SetInputBuffer(body);
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "::SetVersioning", "response: %s\n",
           req->GetOutputAsString().c_str());
    throw std::runtime_error("request failed.");
  }
}

}  // namespace
}  // namespace s3

int main(int argc, char **argv) {
  int opt = 0;
  std::string config_file, in_key_file, out_key_file;
  while ((opt = getopt_long(argc, argv, s3::SHORT_OPTIONS, s3::LONG_OPTIONS,
                            nullptr)) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;

      default:
        s3::PrintUsage(argv[0]);
    }
  }

#define GET_NEXT_ARG(arg)   \
  do {                      \
    if (optind < argc) {    \
      arg = argv[optind++]; \
    }                       \
  } while (0)

  std::string command, state;
  GET_NEXT_ARG(command);
  GET_NEXT_ARG(state);

  int ret = 0;
  try {
    if (command == "versioning") {
      if (state != "on" && state != "off") s3::PrintUsage(argv[0]);
      s3::SetVersioning(config_file, (state == "on"));
    } else if (command == "get-versioning") {
      s3::GetVersioning(config_file);
    } else {
      s3::PrintUsage(argv[0]);
    }
  } catch (const std::exception &e) {
    std::cout << "Caught exception: " << e.what() << std::endl;
    ret = 1;
  }

  return ret;
}
