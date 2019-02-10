#include "base/url.h"

#include <string>

namespace s3 {
namespace base {

std::string Url::Encode(const std::string &url) {
  constexpr char HEX[] = "0123456789ABCDEF";

  std::string ret;
  ret.reserve(url.length());

  for (size_t i = 0; i < url.length(); i++) {
    if (url[i] == '/' || url[i] == '.' || url[i] == '-' || url[i] == '*' ||
        url[i] == '_' || isalnum(url[i]))
      ret += url[i];
    else {
      // allow spaces to be encoded as "%20" rather than "+" because Google
      // Storage doesn't decode the same way AWS does
      ret += '%';
      ret += HEX[static_cast<uint8_t>(url[i]) / 16];
      ret += HEX[static_cast<uint8_t>(url[i]) % 16];
    }
  }

  return ret;
}

}  // namespace base
}  // namespace s3
