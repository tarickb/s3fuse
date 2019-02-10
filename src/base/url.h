#include <string>

namespace s3 {
namespace base {

class Url {
 public:
  static std::string Encode(const std::string &url);
};
}  // namespace base
}  // namespace s3
