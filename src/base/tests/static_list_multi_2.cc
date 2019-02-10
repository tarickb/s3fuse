#include "base/tests/static_list_multi.h"

namespace s3 {
namespace base {
namespace tests {

std::string VoidFunction2() { return "void fn 2"; }

std::string IntFunction2(int a) { return "int fn 2"; }

VoidFunctionList::Entry vf2(VoidFunction2, 100);
IntFunctionList::Entry if2(IntFunction2, 10);

}  // namespace tests
}  // namespace base
}  // namespace s3
