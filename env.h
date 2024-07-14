#ifndef ENV_H
#define ENV_H
#include "types.h"
#include <variant>
namespace env {
struct VarEntry {
  types::Ty ty;
};
struct FunEntry {
  std::vector<types::Ty> formals;
  types::Ty result;
};
using EnvEntry = std::variant<VarEntry, FunEntry>;
using types::as;
using types::is;
} // namespace env
#endif
