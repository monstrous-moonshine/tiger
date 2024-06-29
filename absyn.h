#include "symbol.h"
#include <cstdio>
#include <memory>
#include <vector>

template <typename T> using uptr = std::unique_ptr<T>;

class ExprAST;
using NamedExpr = std::pair<Symbol, uptr<ExprAST>>;

class Ty;
using NamedType = std::pair<Symbol, uptr<Ty>>;

class DeclAST;

enum OpType {
  OP_PLUS,
  OP_MINUS,
  OP_MUL,
  OP_DIV,
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_LE,
  OP_GT,
  OP_GE,
  OP_AND,
  OP_OR,
};

struct Tyfield {
  Symbol name, type_id;
  bool escape{true};

public:
  Tyfield(const char *name, const char *type_id)
      : name(name), type_id(type_id) {}
};

class ExprSeq {
  std::vector<uptr<ExprAST>> seq;
  friend class CallExprAST;
  friend class SeqExprAST;
  friend ExprAST *expseq_to_expr(ExprSeq *);

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
  std::vector<NamedExpr> seq;
  friend class RecordExprAST;

public:
  FieldSeq() = default;
  void AddField(NamedExpr *field) {
    seq.push_back(std::move(*field));
    delete field;
  }
};

class TyfieldSeq {
  std::vector<Tyfield> seq;
  friend class RecordTy;
  friend class FundecTy;

public:
  TyfieldSeq() = default;
  void AddField(Tyfield *field) {
    seq.push_back(*field);
    delete field;
  }
};

class Object {
protected:
  void do_indent(int indent) {
    for (int i = 0; i < indent; i++)
      std::printf(" ");
  }

public:
  virtual ~Object() = default;
  virtual void print(int indent) = 0;
};

class ExprAST : public Object {
public:
  virtual ~ExprAST() = default;
};

class VarExprAST : public ExprAST {
public:
  virtual ~VarExprAST() = default;
};

class SimpleVarExprAST : public VarExprAST {
  Symbol id;

public:
  SimpleVarExprAST(const char *id) : id(id) {}
  void print(int) override { std::printf("%s", id.str()); }
};

class FieldVarExprAST : public VarExprAST {
  uptr<VarExprAST> var;
  Symbol field;

public:
  FieldVarExprAST(ExprAST *var, const char *field)
      : var(static_cast<VarExprAST *>(var)), field(field) {}
  void print(int indent) override {
    var->print(indent);
    std::printf(".%s", field.str());
  }
};

class IndexVarExprAST : public VarExprAST {
  uptr<VarExprAST> var;
  uptr<ExprAST> index;

public:
  IndexVarExprAST(ExprAST *var, ExprAST *index)
      : var(static_cast<VarExprAST *>(var)), index(index) {}
  void print(int indent) override {
    var->print(indent);
    std::printf("[");
    index->print(indent);
    std::printf("]");
  }
};

class NilExprAST : public ExprAST {
public:
  void print(int) override { std::printf("nil"); }
};

class IntExprAST : public ExprAST {
  int val;

public:
  IntExprAST(int val) : val(val) {}
  void print(int) override { std::printf("%d", val); }
};

class StringExprAST : public ExprAST {
  Symbol val;

public:
  StringExprAST(const char *val) : val(val) {}
  void print(int) override { std::printf("%s", val.str()); }
};

class CallExprAST : public ExprAST {
  Symbol fn;
  std::vector<uptr<ExprAST>> args;

public:
  CallExprAST(const char *fn, ExprSeq *args)
      : fn(fn), args(std::move(args->seq)) {
    delete args;
  }
  void print(int indent) override {
    std::printf("%s(", fn.str());
    const char *sep = "";
    for (const auto &arg : args) {
      std::printf("%s", sep);
      arg->print(indent);
      sep = ", ";
    }
    std::printf(")");
  }
};

class OpExprAST : public ExprAST {
  uptr<ExprAST> lhs, rhs;
  OpType op;

public:
  OpExprAST(ExprAST *lhs, ExprAST *rhs, OpType op)
      : lhs(lhs), rhs(rhs), op(op) {}
  void print(int indent) override {
    const char *op_str[] = {
        "+", "-", "*", "/", "=", "<>", "<", "<=", ">", ">=", "&", "|",
    };
    std::printf("(");
    // this is a bit arbitrary
    lhs->print(indent + 2);
    std::printf("%s", op_str[op]);
    rhs->print(indent + 2);
    std::printf(")");
  }
};

class RecordExprAST : public ExprAST {
  Symbol type_id;
  std::vector<NamedExpr> args;

public:
  RecordExprAST(const char *type_id, FieldSeq *args)
      : type_id(type_id), args(std::move(args->seq)) {
    delete args;
  }
  void print(int indent) override {
    std::printf("%s {", type_id.str());
    const char *sep = "";
    for (const auto &field : args) {
      std::printf("%s%s=", sep, field.first.str());
      field.second->print(indent);
      sep = ", ";
    }
    std::printf("}");
  }
};

class ArrayExprAST : public ExprAST {
  Symbol type_id;
  uptr<ExprAST> size, init;

public:
  ArrayExprAST(const char *type_id, ExprAST *size, ExprAST *init)
      : type_id(type_id), size(size), init(init) {}
  void print(int indent) override {
    std::printf("%s [", type_id.str());
    size->print(indent);
    std::printf("]");
    std::printf(" of ");
    init->print(indent);
  }
};

class SeqExprAST : public ExprAST {
  std::vector<uptr<ExprAST>> exps;

public:
  SeqExprAST(ExprSeq *exps) : exps(std::move(exps->seq)) { delete exps; }
  void print(int indent) override {
    std::printf("(");
    const char *sep = "";
    for (const auto &exp : exps) {
      std::printf("%s", sep);
      exp->print(indent);
      sep = "; ";
    }
    std::printf(")");
  }
};

class AssignExprAST : public ExprAST {
  uptr<VarExprAST> var;
  uptr<ExprAST> exp;

public:
  AssignExprAST(ExprAST *var, ExprAST *exp)
      : var(static_cast<VarExprAST *>(var)), exp(exp) {}
  void print(int indent) override {
    var->print(indent);
    std::printf(" := ");
    exp->print(indent);
  }
};

class IfExprAST : public ExprAST {
  uptr<ExprAST> cond, then, else_;

public:
  IfExprAST(ExprAST *cond, ExprAST *then, ExprAST *else_)
      : cond(cond), then(then), else_(else_) {}
  void print(int indent) override {
    std::printf("if ");
    cond->print(indent);
    std::printf(" then ");
    then->print(indent);
    if (else_) {
      std::printf(" else ");
      else_->print(indent);
    }
  }
};

class WhileExprAST : public ExprAST {
  uptr<ExprAST> cond, body;

public:
  WhileExprAST(ExprAST *cond, ExprAST *body) : cond(cond), body(body) {}
  void print(int indent) override {
    std::printf("while ");
    cond->print(indent);
    std::printf(" do ");
    body->print(indent);
  }
};

class ForExprAST : public ExprAST {
  Symbol var;
  uptr<ExprAST> lo, hi, body;
  bool escape{};

public:
  ForExprAST(const char *var, ExprAST *lo, ExprAST *hi, ExprAST *body)
      : var(var), lo(lo), hi(hi), body(body) {}
  void print(int indent) override {
    std::printf("for %s := ", var.str());
    lo->print(indent);
    std::printf(" to ");
    hi->print(indent);
    std::printf(" do ");
    body->print(indent);
  }
};

class BreakExprAST : public ExprAST {
  void print(int) override { std::printf("break"); }
};

class DeclAST : public Object {
public:
  virtual ~DeclAST() = default;
};

class LetExprAST : public ExprAST {
  std::vector<uptr<DeclAST>> decs;
  uptr<ExprAST> exp;

public:
  LetExprAST(DeclSeq *decs, ExprAST *exp)
      : decs(std::move(decs->seq)), exp(exp) {
    delete decs;
  }
  void print(int indent) override {
    std::printf("let\n");
    for (auto &dec : decs) {
      do_indent(indent + 4);
      dec->print(indent + 4);
    }
    do_indent(indent + 2);
    std::printf("in\n");
    do_indent(indent + 4);
    exp->print(indent + 4);
    std::printf("\n");
    do_indent(indent);
    std::printf("end\n");
  }
};

class Ty : public Object {
public:
  virtual ~Ty() = default;
};

class NameTy : public Ty {
  Symbol type_id;

public:
  NameTy(const char *id) : type_id(id) {}
  void print(int) override { std::printf("%s", type_id.str()); }
};

class RecordTy : public Ty {
  std::vector<Tyfield> fields;

public:
  RecordTy(TyfieldSeq *fields) : fields(std::move(fields->seq)) {
    delete fields;
  }
  void print(int) override {
    std::printf("{");
    const char *sep = "";
    for (const auto &field : fields) {
      std::printf("%s%s: %s", sep, field.name.str(), field.type_id.str());
      sep = ", ";
    }
    std::printf("}");
  }
};

class ArrayTy : public Ty {
  Symbol type_id;

public:
  ArrayTy(const char *id) : type_id(id) {}
  void print(int) override { std::printf("array of %s", type_id.str()); }
};

class TypeDeclAST : public DeclAST {
  std::vector<NamedType> types;

public:
  TypeDeclAST() = default;
  void AddType(NamedType *type) {
    types.push_back(std::move(*type));
    delete type;
  }
  void print(int indent) override {
    bool needs_indent = false;
    for (auto &type : types) {
      if (needs_indent)
        do_indent(indent);
      std::printf("type %s = ", type.first.str());
      type.second->print(indent);
      std::printf("\n");
      needs_indent = true;
    }
  }
};

class VarDeclAST : public DeclAST {
  Symbol name, type_id;
  bool escape{true};
  uptr<ExprAST> init;

public:
  VarDeclAST(const char *name, const char *type_id, ExprAST *init)
      : name(name), type_id(type_id), init(init) {}
  void print(int indent) override {
    std::printf("var %s", name.str());
    if (type_id)
      std::printf(": %s", type_id.str());
    std::printf(" := ");
    init->print(indent);
    std::printf("\n");
  }
};

class FundecTy {
  Symbol name, result;
  std::vector<Tyfield> params;
  uptr<ExprAST> body;

public:
  FundecTy(const char *name, const char *result, TyfieldSeq *params,
           ExprAST *body)
      : name(name), result(result), params(std::move(params->seq)), body(body) {
    delete params;
  }
  void print(int indent) {
    std::printf("function %s(", name.str());
    const char *sep = "";
    for (const auto &param : params) {
      std::printf("%s%s: %s", sep, param.name.str(), param.type_id.str());
      sep = ", ";
    }
    std::printf(")");
    if (result) std::printf(" : %s", result.str());
    std::printf(" =\n");
    body->print(indent + 2);
  }
};

class FuncDeclAST : public DeclAST {
  std::vector<FundecTy> decls;

public:
  FuncDeclAST() = default;
  void AddFunc(FundecTy *decl) { decls.push_back(std::move(*decl)); delete decl; }
  void print(int indent) override {
    for (auto& decl: decls) {
      decl.print(indent);
      std::printf("\n");
    }
  }
};
