#ifndef ABSYN_H
#define ABSYN_H
#include "absyn_common.h"
#include "location.h"
#include "symbol.h"
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

struct RExprField {
  Symbol name;
  ExprAST value;
  Location pos;

  RExprField(const char *name, ExprAST *value, Location pos)
      : name(name), value(std::move(*value)), pos(pos) {}
};

struct Type {
  Symbol name;
  Ty type;
  Location pos;

  Type(const char *name, Ty *type, Location pos)
      : name(name), type(std::move(*type)), pos(pos) {}
};

struct RTyField {
  Symbol name, type_id;
  bool escape{true};
  Location pos;

  RTyField(const char *name, const char *type_id, Location pos)
      : name(name), type_id(type_id), pos(pos) {}
};

struct ExprWithLoc {
  ExprAST exp;
  Location pos;
};

struct SymbolWithLoc {
  Symbol sym;
  Location pos;
};

// Temporary classes for AST building convenience {{{
class ExprSeq {
  std::vector<ExprWithLoc> seq;
  friend class CallExprAST;
  friend class SeqExprAST;
  friend ExprAST *yy::expseq_to_expr(ExprSeq *);

public:
  ExprSeq() = default;
  void AddExpr(ExprAST *exp, Location pos) {
    seq.push_back({std::move(*exp), pos});
  }
};

class DeclSeq {
  std::vector<DeclAST> seq;
  friend class LetExprAST;

public:
  DeclSeq() = default;
  void AddDecl(DeclAST *decl) { seq.push_back(std::move(*decl)); }
};

class RExprFieldSeq {
  std::vector<RExprField> seq;
  friend class RecordExprAST;

public:
  RExprFieldSeq() = default;
  void AddField(RExprField *field) {
    seq.push_back(std::move(*field));
    delete field;
  }
};

class RTyFieldSeq {
  std::vector<RTyField> seq;
  friend class RecordTy;
  friend class FundecTy;

public:
  RTyFieldSeq() = default;
  void AddField(RTyField *field) {
    seq.push_back(*field);
    delete field;
  }
};
// }}} Temporary classes for AST building convenience

struct SimpleVarAST {
  Symbol id;
  Location pos;

  SimpleVarAST(const char *id, Location pos) : id(id), pos(pos) {}
};

struct FieldVarAST {
  VarAST var;
  Symbol field;
  Location pos;

  FieldVarAST(VarAST *var, const char *field, Location pos)
      : var(std::move(*var)), field(field), pos(pos) {}
};

struct IndexVarAST {
  VarAST var;
  ExprAST index;
  Location pos;

  IndexVarAST(VarAST *var, ExprAST *index, Location pos)
      : var(std::move(*var)), index(std::move(*index)), pos(pos) {}
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
  std::vector<ExprWithLoc> args;
  Location pos;

  CallExprAST(const char *fn, ExprSeq *args, Location pos)
      : func(fn), args(std::move(args->seq)), pos(pos) {
    delete args;
  }
};

struct OpExprAST : ExprAST {
  ExprAST lhs, rhs;
  Op op;
  Location pos;

  OpExprAST(ExprAST *lhs, ExprAST *rhs, Op op, Location pos)
      : lhs(std::move(*lhs)), rhs(std::move(*rhs)), op(op), pos(pos) {}
};

struct RecordExprAST : ExprAST {
  Symbol type_id;
  std::vector<RExprField> fields;
  Location pos;

  RecordExprAST(const char *type_id, RExprFieldSeq *args, Location pos)
      : type_id(type_id), fields(std::move(args->seq)), pos(pos) {
    delete args;
  }
};

struct ArrayExprAST : ExprAST {
  Symbol type_id;
  ExprAST size, init;
  Location pos;

  ArrayExprAST(const char *type_id, ExprAST *size, ExprAST *init, Location pos)
      : type_id(type_id), size(std::move(*size)), init(std::move(*init)),
        pos(pos) {}
};

struct SeqExprAST : ExprAST {
  std::vector<ExprWithLoc> exps;

  SeqExprAST(ExprSeq *exps) : exps(std::move(exps->seq)) { delete exps; }
};

struct AssignExprAST : ExprAST {
  VarAST var;
  ExprAST exp;
  Location pos;

  AssignExprAST(VarAST *var, ExprAST *exp, Location pos)
      : var(std::move(*var)), exp(std::move(*exp)), pos(pos) {}
};

struct IfExprAST : ExprAST {
  ExprAST cond, then;
  std::optional<ExprAST> else_;
  Location pos;

  IfExprAST(ExprAST *cond, ExprAST *then, ExprAST *else_, Location pos)
      : cond(std::move(*cond)), then(std::move(*then)),
        else_(else_ ? std::move(*else_) : std::optional<ExprAST>{}), pos(pos) {}
};

struct WhileExprAST : ExprAST {
  ExprAST cond, body;
  Location pos;

  WhileExprAST(ExprAST *cond, ExprAST *body, Location pos)
      : cond(std::move(*cond)), body(std::move(*body)), pos(pos) {}
};

struct ForExprAST : ExprAST {
  Symbol var;
  ExprAST lo, hi, body;
  bool escape{true};
  Location pos;

  ForExprAST(const char *var, ExprAST *lo, ExprAST *hi, ExprAST *body,
             Location pos)
      : var(var), lo(std::move(*lo)), hi(std::move(*hi)),
        body(std::move(*body)), pos(pos) {}
};

struct BreakExprAST : ExprAST {
  Location pos;

  BreakExprAST(Location pos) : pos(pos) {}
};

struct LetExprAST : ExprAST {
  std::vector<DeclAST> decs;
  ExprAST body;
  Location pos;

  LetExprAST(DeclSeq *decs, ExprAST *exp, Location pos)
      : decs(std::move(decs->seq)), body(std::move(*exp)), pos(pos) {
    delete decs;
  }
};

struct UnitExprAST : ExprAST {};

struct NameTy {
  Symbol type_id;
  Location pos;

  NameTy(const char *id, Location pos) : type_id(id), pos(pos) {}
};

struct RecordTy {
  std::vector<RTyField> fields;

  RecordTy(RTyFieldSeq *fields) : fields(std::move(fields->seq)) {
    delete fields;
  }
};

struct ArrayTy {
  Symbol type_id;
  Location pos;

  ArrayTy(const char *id, Location pos) : type_id(id), pos(pos) {}
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
  Symbol name;
  std::optional<SymbolWithLoc> type_id;
  bool escape{true};
  ExprAST init;
  Location pos;

  VarDeclAST(const char *name, const char *type_id, Location pos_typ,
             ExprAST *init, Location pos)
      : name(name), type_id(type_id ? SymbolWithLoc{Symbol(type_id), pos_typ}
                                    : std::optional<SymbolWithLoc>{}),
        init(std::move(*init)), pos(pos) {}
};

struct FundecTy {
  Symbol name;
  std::vector<RTyField> params;
  std::optional<SymbolWithLoc> result;
  ExprAST body;
  Location pos;

  FundecTy(const char *name, RTyFieldSeq *params, const char *result,
           Location pos_res, ExprAST *body, Location pos)
      : name(name), params(std::move(params->seq)),
        result(result ? SymbolWithLoc{Symbol(result), pos_res}
                      : std::optional<SymbolWithLoc>{}),
        body(std::move(*body)), pos(pos) {
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
