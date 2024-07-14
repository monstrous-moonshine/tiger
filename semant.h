#ifndef SEMANT_H
#define SEMANT_H
#include "absyn.h"
#include "env.h"
#include "symbol.h"
#include "types.h"
namespace semant {
using Venv = symbol::Table<env::EnvEntry>;
using Tenv = symbol::Table<types::Ty>;
struct Expty {
  types::Ty ty;
};

Expty trans_exp(Venv &, Tenv &, absyn::ExprAST &);
} // namespace semant
#endif
