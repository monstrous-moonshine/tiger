#ifndef PRINT_H
#define PRINT_H
#include "absyn.h"

namespace absyn {

inline void do_indent(int indent) {
  for (int i = 0; i < indent; i++)
    std::printf(" ");
}

class VarASTPrintVisitor {
  int indent_;

public:
  VarASTPrintVisitor(int indent) : indent_(indent) {}
  void operator()(uptr<SimpleVarAST> &var) { std::printf("%s", var->id.name()); }
  void operator()(uptr<FieldVarAST> &var) {
    std::visit(VarASTPrintVisitor(indent_), var->var);
    std::printf(".%s", var->field.name());
  }
  void operator()(uptr<IndexVarAST> &var);
};

class ExprASTPrintVisitor {
  int indent_;

public:
  ExprASTPrintVisitor(int indent) : indent_(indent) {}
  void operator()(uptr<VarExprAST> &e) {
    std::visit(VarASTPrintVisitor(indent_), e->var);
  }
  void operator()(uptr<NilExprAST> &) { std::printf("nil"); }
  void operator()(uptr<IntExprAST> &e) { std::printf("%d", e->val); }
  void operator()(uptr<StringExprAST> &e) { std::printf("%s", e->val.name()); }
  void operator()(uptr<CallExprAST> &e) {
    std::printf("%s(", e->fn.name());
    const char *sep = "";
    for (const auto &arg : e->args) {
      std::printf("%s", sep);
      std::visit(ExprASTPrintVisitor(indent_), *arg);
      sep = ", ";
    }
    std::printf(")");
  }
  void operator()(uptr<OpExprAST> &e) {
    const char *op_str[] = {
        "+", "-", "*", "/", "=", "<>", "<", "<=", ">", ">=", "&", "|",
    };
    std::printf("(");
    // this is a bit arbitrary
    std::visit(ExprASTPrintVisitor(indent_ + 2), *e->lhs);
    std::printf("%s", op_str[static_cast<int>(e->op)]);
    std::visit(ExprASTPrintVisitor(indent_ + 2), *e->rhs);
    std::printf(")");
  }
  void operator()(uptr<RecordExprAST> &e) {
    std::printf("%s {", e->type_id.name());
    const char *sep = "";
    for (const auto &field : e->args) {
      std::printf("%s%s=", sep, field.first.name());
      std::visit(ExprASTPrintVisitor(indent_), *field.second);
      sep = ", ";
    }
    std::printf("}");
  }
  void operator()(uptr<ArrayExprAST> &e) {
    std::printf("%s [", e->type_id.name());
    std::visit(ExprASTPrintVisitor(indent_), *e->size);
    std::printf("]");
    std::printf(" of ");
    std::visit(ExprASTPrintVisitor(indent_), *e->init);
  }
  void operator()(uptr<SeqExprAST> &e) {
    std::printf("(");
    const char *sep = "";
    for (const auto &exp : e->exps) {
      std::printf("%s", sep);
      ExprASTPrintVisitor v(indent_);
      std::visit(ExprASTPrintVisitor(indent_), *exp);
      sep = "; ";
    }
    std::printf(")");
  }
  void operator()(uptr<AssignExprAST> &e) {
    std::visit(VarASTPrintVisitor(indent_), e->var);
    std::printf(" := ");
    ExprASTPrintVisitor ev(indent_);
    std::visit(ExprASTPrintVisitor(indent_), *e->exp);
  }
  void operator()(uptr<IfExprAST> &e) {
    std::printf("if ");
    ExprASTPrintVisitor v(indent_);
    std::visit(ExprASTPrintVisitor(indent_), *e->cond);
    std::printf(" then ");
    std::visit(ExprASTPrintVisitor(indent_), *e->then);
    if (e->else_) {
      std::printf(" else ");
      std::visit(ExprASTPrintVisitor(indent_), *e->else_);
    }
  }
  void operator()(uptr<WhileExprAST> &e) {
    std::printf("while ");
    ExprASTPrintVisitor v(indent_);
    std::visit(ExprASTPrintVisitor(indent_), *e->cond);
    std::printf(" do ");
    std::visit(ExprASTPrintVisitor(indent_), *e->body);
  }
  void operator()(uptr<ForExprAST> &e) {
    std::printf("for %s := ", e->var.name());
    ExprASTPrintVisitor v(indent_);
    std::visit(ExprASTPrintVisitor(indent_), *e->lo);
    std::printf(" to ");
    std::visit(ExprASTPrintVisitor(indent_), *e->hi);
    std::printf(" do ");
    std::visit(ExprASTPrintVisitor(indent_), *e->body);
  }
  void operator()(uptr<BreakExprAST> &) { std::printf("break"); }
  void operator()(uptr<LetExprAST> &e);
};

inline void VarASTPrintVisitor::operator()(uptr<IndexVarAST> &var) {
  std::visit(VarASTPrintVisitor(indent_), var->var);
  std::printf("[");
  std::visit(ExprASTPrintVisitor(indent_), *var->index);
  std::printf("]");
}

class TyPrintVisitor {
  int indent_;

public:
  TyPrintVisitor(int indent) : indent_(indent) {}
  void operator()(uptr<NameTy> &ty) { std::printf("%s", ty->type_id.name()); }
  void operator()(uptr<RecordTy> &ty) {
    std::printf("{");
    const char *sep = "";
    for (const auto &field : ty->fields) {
      std::printf("%s%s: %s", sep, field.name.name(), field.type_id.name());
      sep = ", ";
    }
    std::printf("}");
  }
  void operator()(uptr<ArrayTy> &ty) {
    std::printf("array of %s", ty->type_id.name());
  }
};

class DeclASTPrintVisitor {
  int indent_;

public:
  DeclASTPrintVisitor(int indent) : indent_(indent) {}
  void operator()(uptr<TypeDeclAST> &decl) {
    bool needs_indent = false;
    for (auto &type : decl->types) {
      if (needs_indent)
        do_indent(indent_);
      std::printf("type %s = ", type.first.name());
      TyPrintVisitor v(indent_);
      std::visit(TyPrintVisitor(indent_), type.second);
      std::printf("\n");
      needs_indent = true;
    }
  }
  void operator()(uptr<VarDeclAST> &decl) {
    std::printf("var %s", decl->name.name());
    if (decl->type_id)
      std::printf(": %s", decl->type_id.name());
    std::printf(" := ");
    std::visit(ExprASTPrintVisitor(indent_), *decl->init);
    std::printf("\n");
  }
  void operator()(uptr<FuncDeclAST> &decl) {
    for (auto &decl_ : decl->decls) {
      decl_.print(indent_);
      std::printf("\n");
    }
  }
};

inline void ExprASTPrintVisitor::operator()(uptr<LetExprAST> &e) {
  std::printf("let\n");
  for (auto &dec : e->decs) {
    do_indent(indent_ + 4);
    std::visit(DeclASTPrintVisitor(indent_ + 4), dec);
  }
  do_indent(indent_ + 2);
  std::printf("in\n");
  if (e->exp) {
    do_indent(indent_ + 4);
    std::visit(ExprASTPrintVisitor(indent_ + 4), *e->exp);
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
  std::visit(ExprASTPrintVisitor(indent + 2), *body);
}

} // namespace absyn
#endif
