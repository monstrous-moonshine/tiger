#ifndef PRINT_H
#define PRINT_H
#include "absyn.h"

namespace absyn {

inline void do_indent(int indent) {
  for (int i = 0; i < indent; i++)
    std::printf(" ");
}

class VarASTPrintVisitor : public VarASTVisitor {
  int indent_;

public:
  VarASTPrintVisitor(int indent) : indent_(indent) {}
  void visit(SimpleVarAST &var) override { std::printf("%s", var.id.name()); }
  void visit(FieldVarAST &var) override {
    var.var->accept(*this);
    std::printf(".%s", var.field.name());
  }
  void visit(IndexVarAST &var) override;
};

class ExprASTPrintVisitor : public ExprASTVisitor {
  int indent_;

public:
  ExprASTPrintVisitor(int indent) : indent_(indent) {}
  virtual ~ExprASTPrintVisitor() = default;
  void visit(VarExprAST &e) override {
    VarASTPrintVisitor v(indent_);
    e.var->accept(v);
  }
  void visit(NilExprAST &) override { std::printf("nil"); }
  void visit(IntExprAST &e) override { std::printf("%d", e.val); }
  void visit(StringExprAST &e) override { std::printf("%s", e.val.name()); }
  void visit(CallExprAST &e) override {
    std::printf("%s(", e.fn.name());
    const char *sep = "";
    for (const auto &arg : e.args) {
      std::printf("%s", sep);
      ExprASTPrintVisitor v(indent_);
      arg->accept(v);
      sep = ", ";
    }
    std::printf(")");
  }
  void visit(OpExprAST &e) override {
    const char *op_str[] = {
        "+", "-", "*", "/", "=", "<>", "<", "<=", ">", ">=", "&", "|",
    };
    std::printf("(");
    // this is a bit arbitrary
    ExprASTPrintVisitor v(indent_ + 2);
    e.lhs->accept(v);
    std::printf("%s", op_str[static_cast<int>(e.op)]);
    e.rhs->accept(v);
    std::printf(")");
  }
  void visit(RecordExprAST &e) override {
    std::printf("%s {", e.type_id.name());
    const char *sep = "";
    for (const auto &field : e.args) {
      std::printf("%s%s=", sep, field.first.name());
      ExprASTPrintVisitor v(indent_);
      field.second->accept(v);
      sep = ", ";
    }
    std::printf("}");
  }
  void visit(ArrayExprAST &e) override {
    std::printf("%s [", e.type_id.name());
    ExprASTPrintVisitor v(indent_);
    e.size->accept(v);
    std::printf("]");
    std::printf(" of ");
    e.init->accept(v);
  }
  void visit(SeqExprAST &e) override {
    std::printf("(");
    const char *sep = "";
    for (const auto &exp : e.exps) {
      std::printf("%s", sep);
      ExprASTPrintVisitor v(indent_);
      exp->accept(v);
      sep = "; ";
    }
    std::printf(")");
  }
  void visit(AssignExprAST &e) override {
    VarASTPrintVisitor v(indent_);
    e.var->accept(v);
    std::printf(" := ");
    ExprASTPrintVisitor ev(indent_);
    e.exp->accept(ev);
  }
  void visit(IfExprAST &e) override {
    std::printf("if ");
    ExprASTPrintVisitor v(indent_);
    e.cond->accept(v);
    std::printf(" then ");
    e.then->accept(v);
    if (e.else_) {
      std::printf(" else ");
      e.else_->accept(v);
    }
  }
  void visit(WhileExprAST &e) override {
    std::printf("while ");
    ExprASTPrintVisitor v(indent_);
    e.cond->accept(v);
    std::printf(" do ");
    e.body->accept(v);
  }
  void visit(ForExprAST &e) override {
    std::printf("for %s := ", e.var.name());
    ExprASTPrintVisitor v(indent_);
    e.lo->accept(v);
    std::printf(" to ");
    e.hi->accept(v);
    std::printf(" do ");
    e.body->accept(v);
  }
  void visit(BreakExprAST &) override { std::printf("break"); }
  void visit(LetExprAST &e) override;
};

inline void VarASTPrintVisitor::visit(IndexVarAST &var) {
  var.var->accept(*this);
  std::printf("[");
  ExprASTPrintVisitor v(indent_);
  var.index->accept(v);
  std::printf("]");
}

class TyPrintVisitor : public TyVisitor {
  int indent_;

public:
  TyPrintVisitor(int indent) : indent_(indent) {}
  void visit(NameTy &ty) override { std::printf("%s", ty.type_id.name()); }
  void visit(RecordTy &ty) override {
    std::printf("{");
    const char *sep = "";
    for (const auto &field : ty.fields) {
      std::printf("%s%s: %s", sep, field.name.name(), field.type_id.name());
      sep = ", ";
    }
    std::printf("}");
  }
  void visit(ArrayTy &ty) override {
    std::printf("array of %s", ty.type_id.name());
  }
};

class DeclASTPrintVisitor : public DeclASTVisitor {
  int indent_;

public:
  DeclASTPrintVisitor(int indent) : indent_(indent) {}
  void visit(TypeDeclAST &decl) override {
    bool needs_indent = false;
    for (auto &type : decl.types) {
      if (needs_indent)
        do_indent(indent_);
      std::printf("type %s = ", type.first.name());
      TyPrintVisitor v(indent_);
      type.second->accept(v);
      std::printf("\n");
      needs_indent = true;
    }
  }
  void visit(VarDeclAST &decl) override {
    std::printf("var %s", decl.name.name());
    if (decl.type_id)
      std::printf(": %s", decl.type_id.name());
    std::printf(" := ");
    ExprASTPrintVisitor v(indent_);
    decl.init->accept(v);
    std::printf("\n");
  }
  void visit(FuncDeclAST &decl) override {
    for (auto &decl_ : decl.decls) {
      decl_.print(indent_);
      std::printf("\n");
    }
  }
};

inline void ExprASTPrintVisitor::visit(LetExprAST &e) {
  std::printf("let\n");
  for (auto &dec : e.decs) {
    do_indent(indent_ + 4);
    DeclASTPrintVisitor v(indent_ + 4);
    dec->accept(v);
  }
  do_indent(indent_ + 2);
  std::printf("in\n");
  if (e.exp) {
    do_indent(indent_ + 4);
    ExprASTPrintVisitor v(indent_ + 4);
    e.exp->accept(v);
    std::printf("\n");
  }
  do_indent(indent_);
  std::printf("end\n");
}

inline void FundecTy::print(int indent) {
  std::printf("function %s(", name.name());
  const char *sep = "";
  for (const auto &param : params) {
    std::printf("%s%s: %s", sep, param.name.name(), param.type_id.name());
    sep = ", ";
  }
  std::printf(")");
  if (result)
    std::printf(" : %s", result.name());
  std::printf(" =\n");
  ExprASTPrintVisitor v(indent + 2);
  body->accept(v);
}

} // namespace absyn
#endif
