#ifndef ABSYN_H
#define ABSYN_H
#include "symbol.h"
#include <cstdio>
#include <memory>
#include <variant>
#include <vector>

template <typename T> using uptr = std::unique_ptr<T>;

namespace absyn {
struct ExprAST;
struct ExprSeq;
} // namespace absyn

namespace yy {
absyn::ExprAST *expseq_to_expr(absyn::ExprSeq *);
}

namespace absyn {

struct SimpleVarAST;
struct FieldVarAST;
struct IndexVarAST;
using VarAST = std::variant<uptr<SimpleVarAST>, uptr<FieldVarAST>, uptr<IndexVarAST>>;

struct NameTy;
struct RecordTy;
struct ArrayTy;
using Ty = std::variant<uptr<NameTy>, uptr<RecordTy>, uptr<ArrayTy>>;

struct TypeDeclAST;
struct VarDeclAST;
struct FuncDeclAST;
using DeclAST = std::variant<uptr<TypeDeclAST>, uptr<VarDeclAST>, uptr<FuncDeclAST>>;

using symbol::Symbol;
using Field = std::pair<Symbol, uptr<ExprAST>>;
using Type = std::pair<Symbol, Ty>;

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
  std::vector<uptr<ExprAST>> seq;
  friend class CallExprAST;
  friend class SeqExprAST;
  friend ExprAST *yy::expseq_to_expr(ExprSeq *);

public:
  ExprSeq() = default;
  void AddExpr(ExprAST *exp) { seq.push_back(uptr<ExprAST>(exp)); }
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

struct VarExprAST;
struct NilExprAST;
struct IntExprAST;
struct StringExprAST;
struct CallExprAST;
struct OpExprAST;
struct RecordExprAST;
struct ArrayExprAST;
struct SeqExprAST;
struct AssignExprAST;
struct IfExprAST;
struct WhileExprAST;
struct ForExprAST;
struct BreakExprAST;
struct LetExprAST;

class ExprASTVisitor {
public:
  virtual ~ExprASTVisitor() = default;
  virtual void visit(VarExprAST &) = 0;
  virtual void visit(NilExprAST &) = 0;
  virtual void visit(IntExprAST &) = 0;
  virtual void visit(StringExprAST &) = 0;
  virtual void visit(CallExprAST &) = 0;
  virtual void visit(OpExprAST &) = 0;
  virtual void visit(RecordExprAST &) = 0;
  virtual void visit(ArrayExprAST &) = 0;
  virtual void visit(SeqExprAST &) = 0;
  virtual void visit(AssignExprAST &) = 0;
  virtual void visit(IfExprAST &) = 0;
  virtual void visit(WhileExprAST &) = 0;
  virtual void visit(ForExprAST &) = 0;
  virtual void visit(BreakExprAST &) = 0;
  virtual void visit(LetExprAST &) = 0;
};

class DeclASTVisitor {
public:
  virtual ~DeclASTVisitor() = default;
  virtual void visit(TypeDeclAST &) = 0;
  virtual void visit(VarDeclAST &) = 0;
  virtual void visit(FuncDeclAST &) = 0;
};

struct SimpleVarAST {
  Symbol id;

  SimpleVarAST(const char *id) : id(id) {}
};

struct FieldVarAST {
  VarAST var;
  Symbol field;

  FieldVarAST(VarAST *var, const char *field) : var(std::move(*var)), field(field) {}
};

struct IndexVarAST {
  VarAST var;
  uptr<ExprAST> index;

  IndexVarAST(VarAST *var, ExprAST *index) : var(std::move(*var)), index(index) {}
};

struct ExprAST {
  virtual ~ExprAST() = default;
  virtual void accept(ExprASTVisitor &) = 0;
};

struct VarExprAST : ExprAST {
  VarAST var;

  VarExprAST(VarAST *var) : var(std::move(*var)) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct NilExprAST : ExprAST {
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct IntExprAST : ExprAST {
  int val;

  IntExprAST(int val) : val(val) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct StringExprAST : ExprAST {
  Symbol val;

  StringExprAST(const char *val) : val(val) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct CallExprAST : ExprAST {
  Symbol fn;
  std::vector<uptr<ExprAST>> args;

  CallExprAST(const char *fn, ExprSeq *args)
      : fn(fn), args(std::move(args->seq)) {
    delete args;
  }
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct OpExprAST : ExprAST {
  uptr<ExprAST> lhs, rhs;
  Op op;

  OpExprAST(ExprAST *lhs, ExprAST *rhs, Op op) : lhs(lhs), rhs(rhs), op(op) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct RecordExprAST : ExprAST {
  Symbol type_id;
  std::vector<Field> args;

  RecordExprAST(const char *type_id, FieldSeq *args)
      : type_id(type_id), args(std::move(args->seq)) {
    delete args;
  }
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct ArrayExprAST : ExprAST {
  Symbol type_id;
  uptr<ExprAST> size, init;

  ArrayExprAST(const char *type_id, ExprAST *size, ExprAST *init)
      : type_id(type_id), size(size), init(init) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct SeqExprAST : ExprAST {
  std::vector<uptr<ExprAST>> exps;

  SeqExprAST(ExprSeq *exps) : exps(std::move(exps->seq)) { delete exps; }
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct AssignExprAST : ExprAST {
  VarAST var;
  uptr<ExprAST> exp;

  AssignExprAST(VarAST *var, ExprAST *exp) : var(std::move(*var)), exp(exp) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct IfExprAST : ExprAST {
  uptr<ExprAST> cond, then, else_;

  IfExprAST(ExprAST *cond, ExprAST *then, ExprAST *else_)
      : cond(cond), then(then), else_(else_) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct WhileExprAST : ExprAST {
  uptr<ExprAST> cond, body;

  WhileExprAST(ExprAST *cond, ExprAST *body) : cond(cond), body(body) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct ForExprAST : ExprAST {
  Symbol var;
  uptr<ExprAST> lo, hi, body;
  bool escape{true};

  ForExprAST(const char *var, ExprAST *lo, ExprAST *hi, ExprAST *body)
      : var(var), lo(lo), hi(hi), body(body) {}
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct BreakExprAST : ExprAST {
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct LetExprAST : ExprAST {
  std::vector<DeclAST> decs;
  uptr<ExprAST> exp;

  LetExprAST(DeclSeq *decs, ExprAST *exp)
      : decs(std::move(decs->seq)), exp(exp) {
    delete decs;
  }
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

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
  uptr<ExprAST> init;

  VarDeclAST(const char *name, const char *type_id, ExprAST *init)
      : name(name), type_id(type_id), init(init) {}
};

struct FundecTy {
  Symbol name;
  std::vector<FieldTy> params;
  Symbol result;
  uptr<ExprAST> body;

  FundecTy(const char *name, FieldTySeq *params, const char *result,
           ExprAST *body)
      : name(name), params(std::move(params->seq)), result(result), body(body) {
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
