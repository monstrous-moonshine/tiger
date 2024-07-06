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
struct DeclAST;
struct ExprSeq;
struct Ty;
} // namespace absyn

namespace yy {
absyn::ExprAST *expseq_to_expr(absyn::ExprSeq *);
}

namespace absyn {

using symbol::Symbol;
using Field = std::pair<Symbol, uptr<ExprAST>>;
using Type = std::pair<Symbol, uptr<Ty>>;

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
  std::vector<uptr<DeclAST>> seq;
  friend class LetExprAST;

public:
  DeclSeq() = default;
  void AddDecl(DeclAST *decl) { seq.push_back(uptr<DeclAST>(decl)); }
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

struct SimpleVarAST;
struct FieldVarAST;
struct IndexVarAST;

class VarASTVisitor {
public:
  virtual ~VarASTVisitor() = default;
  virtual void visit(SimpleVarAST &) = 0;
  virtual void visit(FieldVarAST &) = 0;
  virtual void visit(IndexVarAST &) = 0;
};

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

struct NameTy;
struct RecordTy;
struct ArrayTy;

class TyVisitor {
public:
  virtual ~TyVisitor() = default;
  virtual void visit(NameTy &) = 0;
  virtual void visit(RecordTy &) = 0;
  virtual void visit(ArrayTy &) = 0;
};

struct TypeDeclAST;
struct VarDeclAST;
struct FuncDeclAST;

class DeclASTVisitor {
public:
  virtual ~DeclASTVisitor() = default;
  virtual void visit(TypeDeclAST &) = 0;
  virtual void visit(VarDeclAST &) = 0;
  virtual void visit(FuncDeclAST &) = 0;
};

struct VarAST {
  virtual ~VarAST() = default;
  virtual void accept(VarASTVisitor &) = 0;
};

struct SimpleVarAST : VarAST {
  Symbol id;

  SimpleVarAST(const char *id) : id(id) {}
  void accept(VarASTVisitor &visitor) override { visitor.visit(*this); }
};

struct FieldVarAST : VarAST {
  uptr<VarAST> var;
  Symbol field;

  FieldVarAST(VarAST *var, const char *field) : var(var), field(field) {}
  void accept(VarASTVisitor &visitor) override { visitor.visit(*this); }
};

struct IndexVarAST : VarAST {
  uptr<VarAST> var;
  uptr<ExprAST> index;

  IndexVarAST(VarAST *var, ExprAST *index) : var(var), index(index) {}
  void accept(VarASTVisitor &visitor) override { visitor.visit(*this); }
};

struct ExprAST {
  virtual ~ExprAST() = default;
  virtual void accept(ExprASTVisitor &) = 0;
};

struct VarExprAST : ExprAST {
  uptr<VarAST> var;

  VarExprAST(VarAST *var) : var(var) {}
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
  uptr<VarAST> var;
  uptr<ExprAST> exp;

  AssignExprAST(VarAST *var, ExprAST *exp) : var(var), exp(exp) {}
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
  std::vector<uptr<DeclAST>> decs;
  uptr<ExprAST> exp;

  LetExprAST(DeclSeq *decs, ExprAST *exp)
      : decs(std::move(decs->seq)), exp(exp) {
    delete decs;
  }
  void accept(ExprASTVisitor &visitor) override { visitor.visit(*this); }
};

struct Ty {
  virtual ~Ty() = default;
  virtual void accept(TyVisitor &) = 0;
};

struct NameTy : Ty {
  Symbol type_id;

  NameTy(const char *id) : type_id(id) {}
  void accept(TyVisitor &visitor) override { visitor.visit(*this); }
};

struct RecordTy : Ty {
  std::vector<FieldTy> fields;

  RecordTy(FieldTySeq *fields) : fields(std::move(fields->seq)) {
    delete fields;
  }
  void accept(TyVisitor &visitor) override { visitor.visit(*this); }
};

struct ArrayTy : Ty {
  Symbol type_id;

  ArrayTy(const char *id) : type_id(id) {}
  void accept(TyVisitor &visitor) override { visitor.visit(*this); }
};

struct DeclAST {
  virtual ~DeclAST() = default;
  virtual void accept(DeclASTVisitor &) = 0;
};

struct TypeDeclAST : DeclAST {
  std::vector<Type> types;

  TypeDeclAST() = default;
  void AddType(Type *type) {
    types.push_back(std::move(*type));
    delete type;
  }
  void accept(DeclASTVisitor &visitor) override { visitor.visit(*this); }
};

struct VarDeclAST : DeclAST {
  Symbol name, type_id;
  bool escape{true};
  uptr<ExprAST> init;

  VarDeclAST(const char *name, const char *type_id, ExprAST *init)
      : name(name), type_id(type_id), init(init) {}
  void accept(DeclASTVisitor &visitor) override { visitor.visit(*this); }
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

struct FuncDeclAST : DeclAST {
  std::vector<FundecTy> decls;

  FuncDeclAST() = default;
  void AddFunc(FundecTy *decl) {
    decls.push_back(std::move(*decl));
    delete decl;
  }
  void accept(DeclASTVisitor &visitor) override { visitor.visit(*this); }
};

} // namespace absyn
#endif
