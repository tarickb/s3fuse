#ifndef S3_SERVICES_GS_VERSIONING_H
#define S3_SERVICES_GS_VERSIONING_H

#include "services/versioning.h"

namespace s3 {
namespace services {
namespace gs {
class Impl;

class Versioning : public services::Versioning {
 public:
  Versioning(Impl *service);

  std::string BuildVersionedUrl(const std::string &base_path,
                                const std::string &version) override;

  std::string ExtractCurrentVersion(base::Request *req) override;

  int FetchAllVersions(VersionFetchOptions options, std::string path,
                       base::Request *req, std::string *out,
                       int *empty_count) override;

 private:
  Impl *service_;
};

}  // namespace gs
}  // namespace services
}  // namespace s3

#endif
