#ifndef S3_BASE_TESTS_STATIC_LIST_MULTI_H
#define S3_BASE_TESTS_STATIC_LIST_MULTI_H

#include <functional>
#include <string>

#include "base/static_list.h"

namespace s3 {
namespace base {
namespace tests {
using void_fn_list = static_list<std::function<std::string()>>;
using int_fn_list = static_list<std::function<std::string(int)>>;
} // namespace tests
} // namespace base
} // namespace s3

#endif
