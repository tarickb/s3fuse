#ifndef S3_SERVICES_VERSIONING_H
#define S3_SERVICES_VERSIONING_H

#include <string>

#include "base/request.h"

namespace s3 {
namespace services {
enum class VersionFetchOptions { NONE, WITH_EMPTIES };

class Versioning {
 public:
  virtual ~Versioning() = default;

  virtual std::string BuildVersionedUrl(const std::string &base_path,
                                        const std::string &version) = 0;

  virtual std::string ExtractCurrentVersion(base::Request *req) = 0;

  virtual int FetchAllVersions(VersionFetchOptions options, std::string path,
                               base::Request *req, std::string *out,
                               int *empty_count) = 0;
};

}  // namespace services
}  // namespace s3

#endif
