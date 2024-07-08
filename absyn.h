#ifndef ABSYN_H
#define ABSYN_H
#include "absyn_common.h"
#include "symbol.h"
#include <cstdio>
#include <memory>
#include <variant>
#include <vector>

namespace yy {
absyn::ExprAST *expseq_to_expr(absyn::ExprSeq *);
}

namespace absyn {
using symbol::Symbol;

enum class Op : int {
  kPlus,
  kMinus,
  kMul,
  kDiv,
  kEq,
  kNeq,
  kLt,
  kLe,
  kGt,
  kGe,
  kAnd,
  kOr,
};

struct FieldTy {
  Symbol name, type_id;
  bool escape{true};

  FieldTy(const char *name, const char *type_id)
      : name(name), type_id(type_id) {}
};

// Temporary classes for AST building convenience {{{
class ExprSeq {
  std::vector<ExprAST> seq;
  friend class CallExprAST;
  friend class SeqExprAST;
  friend ExprAST *yy::expseq_to_expr(ExprSeq *);

public:
  ExprSeq() = default;
  void AddExpr(ExprAST *exp) { seq.push_back(std::move(*exp)); }
};

class DeclSeq {
  std::vector<DeclAST> seq;
  friend class LetExprAST;

public:
  DeclSeq() = default;
  void AddDecl(DeclAST *decl) { seq.push_back(std::move(*decl)); }
};

class FieldSeq {
  std::vector<Field> seq;
  friend class RecordExprAST;

public:
  FieldSeq() = default;
  void AddField(Field *field) {
    seq.push_back(std::move(*field));
    delete field;
  }
};

class FieldTySeq {
  std::vector<FieldTy> seq;
  friend class RecordTy;
  friend class FundecTy;

public:
  FieldTySeq() = default;
  void AddField(FieldTy *field) {
    seq.push_back(*field);
    delete field;
  }
};
// }}} Temporary classes for AST building convenience

struct SimpleVarAST {
  Symbol id;

  SimpleVarAST(const char *id) : id(id) {}
};

struct FieldVarAST {
  VarAST var;
  Symbol field;

  FieldVarAST(VarAST *var, const char *field)
      : var(std::move(*var)), field(field) {}
};

struct IndexVarAST {
  VarAST var;
  ExprAST index;

  IndexVarAST(VarAST *var, ExprAST *index)
      : var(std::move(*var)), index(std::move(*index)) {}
};

struct VarExprAST : ExprAST {
  VarAST var;

  VarExprAST(VarAST *var) : var(std::move(*var)) {}
};

struct NilExprAST : ExprAST {};

struct IntExprAST : ExprAST {
  int val;

  IntExprAST(int val) : val(val) {}
};

struct StringExprAST : ExprAST {
  Symbol val;

  StringExprAST(const char *val) : val(val) {}
};

struct CallExprAST : ExprAST {
  Symbol func;
  std::vector<ExprAST> args;

  CallExprAST(const char *fn, ExprSeq *args)
      : func(fn), args(std::move(args->seq)) {
    delete args;
  }
};

struct OpExprAST : ExprAST {
  ExprAST lhs, rhs;
  Op op;

  OpExprAST(ExprAST *lhs, ExprAST *rhs, Op op)
      : lhs(std::move(*lhs)), rhs(std::move(*rhs)), op(op) {}
};

struct RecordExprAST : ExprAST {
  Symbol type_id;
  std::vector<Field> fields;

  RecordExprAST(const char *type_id, FieldSeq *args)
      : type_id(type_id), fields(std::move(args->seq)) {
    delete args;
  }
};

struct ArrayExprAST : ExprAST {
  Symbol type_id;
  ExprAST size, init;

  ArrayExprAST(const char *type_id, ExprAST *size, ExprAST *init)
      : type_id(type_id), size(std::move(*size)), init(std::move(*init)) {}
};

struct SeqExprAST : ExprAST {
  std::vector<ExprAST> exps;

  SeqExprAST(ExprSeq *exps) : exps(std::move(exps->seq)) { delete exps; }
};

struct AssignExprAST : ExprAST {
  VarAST var;
  ExprAST exp;

  AssignExprAST(VarAST *var, ExprAST *exp)
      : var(std::move(*var)), exp(std::move(*exp)) {}
};

struct IfExprAST : ExprAST {
  ExprAST cond, then;
  std::optional<ExprAST> else_;

  IfExprAST(ExprAST *cond, ExprAST *then, ExprAST *else_)
      : cond(std::move(*cond)), then(std::move(*then)),
        else_(else_ ? std::move(*else_) : std::optional<ExprAST>{}) {}
};

struct WhileExprAST : ExprAST {
  ExprAST cond, body;

  WhileExprAST(ExprAST *cond, ExprAST *body)
      : cond(std::move(*cond)), body(std::move(*body)) {}
};

struct ForExprAST : ExprAST {
  Symbol var;
  ExprAST lo, hi, body;
  bool escape{true};

  ForExprAST(const char *var, ExprAST *lo, ExprAST *hi, ExprAST *body)
      : var(var), lo(std::move(*lo)), hi(std::move(*hi)),
        body(std::move(*body)) {}
};

struct BreakExprAST : ExprAST {};

struct LetExprAST : ExprAST {
  std::vector<DeclAST> decs;
  ExprAST exp;

  LetExprAST(DeclSeq *decs, ExprAST *exp)
      : decs(std::move(decs->seq)), exp(std::move(*exp)) {
    delete decs;
  }
};

struct UnitExprAST : ExprAST {};

struct NameTy {
  Symbol type_id;

  NameTy(const char *id) : type_id(id) {}
};

struct RecordTy {
  std::vector<FieldTy> fields;

  RecordTy(FieldTySeq *fields) : fields(std::move(fields->seq)) {
    delete fields;
  }
};

struct ArrayTy {
  Symbol type_id;

  ArrayTy(const char *id) : type_id(id) {}
};

struct TypeDeclAST {
  std::vector<Type> types;

  TypeDeclAST() = default;
  void AddType(Type *type) {
    types.push_back(std::move(*type));
    delete type;
  }
};

struct VarDeclAST {
  Symbol name, type_id;
  bool escape{true};
  ExprAST init;

  VarDeclAST(const char *name, const char *type_id, ExprAST *init)
      : name(name), type_id(type_id), init(std::move(*init)) {}
};

struct FundecTy {
  Symbol name;
  std::vector<FieldTy> params;
  Symbol result;
  ExprAST body;

  FundecTy(const char *name, FieldTySeq *params, const char *result,
           ExprAST *body)
      : name(name), params(std::move(params->seq)), result(result),
        body(std::move(*body)) {
    delete params;
  }
  void print(int indent);
};

struct FuncDeclAST {
  std::vector<FundecTy> decls;

  FuncDeclAST() = default;
  void AddFunc(FundecTy *decl) {
    decls.push_back(std::move(*decl));
    delete decl;
  }
};

} // namespace absyn
#endif
