#ifndef S3_BASE_TESTS_STATIC_LIST_MULTI_H
#define S3_BASE_TESTS_STATIC_LIST_MULTI_H

#include <functional>
#include <string>

#include "base/static_list.h"

namespace s3 {
namespace base {
namespace tests {
using VoidFunctionList = StaticList<std::function<std::string()>>;
using IntFunctionList = StaticList<std::function<std::string(int)>>;
}  // namespace tests
}  // namespace base
}  // namespace s3

#endif
