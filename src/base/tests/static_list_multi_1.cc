#include "base/tests/static_list_multi.h"

namespace s3 {
namespace base {
namespace tests {

std::string VoidFunction1() { return "void fn 1"; }

std::string IntFunction1(int a) { return "int fn 1"; }

VoidFunctionList::Entry vf1(VoidFunction1, 1);
IntFunctionList::Entry if1(IntFunction1, 1);

}  // namespace tests
}  // namespace base
}  // namespace s3
