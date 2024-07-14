#ifndef PRINT_H
#define PRINT_H
#include "absyn.h"
#include <cstdio>

namespace absyn {

void print(int indent, ExprAST &e);

namespace detail {

void print(int indent, VarAST &v);
void print(int indent, Ty &ty);
void print(int indent, DeclAST &d);

inline void do_indent(int indent) {
  for (int i = 0; i < indent; i++)
    std::printf(" ");
}

class VarASTPrintVisitor {
  int indent_;

public:
  VarASTPrintVisitor(int indent) : indent_(indent) {}
  void operator()(uptr<SimpleVarAST> &var) {
    std::printf("%s", var->id.name());
  }
  void operator()(uptr<FieldVarAST> &var) {
    print(indent_, var->var);
    std::printf(".%s", var->field.name());
  }
  void operator()(uptr<IndexVarAST> &var);
};

class ExprASTPrintVisitor {
  int indent_;
  bool is_let_body;

public:
  ExprASTPrintVisitor(int indent, bool is_let_body = false)
      : indent_(indent), is_let_body(is_let_body) {}
  void operator()(uptr<VarExprAST> &e) { print(indent_, e->var); }
  void operator()(uptr<NilExprAST> &) { std::printf("nil"); }
  void operator()(uptr<IntExprAST> &e) { std::printf("%d", e->val); }
  void operator()(uptr<StringExprAST> &e) { std::printf("%s", e->val.name()); }
  void operator()(uptr<CallExprAST> &e) {
    std::printf("%s(", e->func.name());
    const char *sep = "";
    for (auto &arg : e->args) {
      std::printf("%s", sep);
      print(indent_, arg.exp);
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
    print(indent_ + 2, e->lhs);
    std::printf("%s", op_str[static_cast<int>(e->op)]);
    print(indent_ + 2, e->rhs);
    std::printf(")");
  }
  void operator()(uptr<RecordExprAST> &e) {
    std::printf("%s {", e->type_id.name());
    const char *sep = "";
    for (auto &field : e->fields) {
      std::printf("%s%s=", sep, field.name.name());
      print(indent_, field.value);
      sep = ", ";
    }
    std::printf("}");
  }
  void operator()(uptr<ArrayExprAST> &e) {
    std::printf("%s [", e->type_id.name());
    print(indent_, e->size);
    std::printf("]");
    std::printf(" of ");
    print(indent_, e->init);
  }
  void operator()(uptr<SeqExprAST> &e) {
    std::printf(is_let_body ? "" : "(");
    const char *sep = is_let_body ? "\n" : "; ";
    bool needs_sep = false;
    for (auto &exp : e->exps) {
      if (needs_sep) {
        std::printf("%s", sep);
        if (is_let_body)
          do_indent(indent_);
      }
      print(indent_, exp.exp);
      needs_sep = true;
    }
    std::printf(is_let_body ? "" : ")");
  }
  void operator()(uptr<AssignExprAST> &e) {
    print(indent_, e->var);
    std::printf(" := ");
    print(indent_, e->exp);
  }
  void operator()(uptr<IfExprAST> &e) {
    std::printf("if ");
    print(indent_, e->cond);
    std::printf(" then ");
    print(indent_, e->then);
    if (e->else_) {
      std::printf(" else ");
      print(indent_, e->else_.value());
    }
  }
  void operator()(uptr<WhileExprAST> &e) {
    std::printf("while ");
    print(indent_, e->cond);
    std::printf(" do ");
    print(indent_, e->body);
  }
  void operator()(uptr<ForExprAST> &e) {
    std::printf("for %s := ", e->var.name());
    print(indent_, e->lo);
    std::printf(" to ");
    print(indent_, e->hi);
    std::printf(" do ");
    print(indent_, e->body);
  }
  void operator()(uptr<BreakExprAST> &) { std::printf("break"); }
  void operator()(uptr<LetExprAST> &e);
  void operator()(uptr<UnitExprAST> &e) { std::printf("()"); }
};

inline void VarASTPrintVisitor::operator()(uptr<IndexVarAST> &var) {
  print(indent_, var->var);
  std::printf("[");
  print(indent_, var->index);
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
      std::printf("%s%s : %s", sep, field.name.name(), field.type_id.name());
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
      if (needs_indent) {
        std::printf("\n");
        do_indent(indent_);
      }
      std::printf("type %s = ", type.name.name());
      print(indent_, type.type);
      needs_indent = true;
    }
  }
  void operator()(uptr<VarDeclAST> &decl) {
    std::printf("var %s", decl->name.name());
    if (decl->type_id)
      std::printf(" : %s", decl->type_id->sym.name());
    std::printf(" := ");
    print(indent_, decl->init);
  }
  void operator()(uptr<FuncDeclAST> &decl) {
    bool needs_indent = false;
    for (auto &decl_ : decl->decls) {
      if (needs_indent) {
        std::printf("\n");
        do_indent(indent_);
      }
      decl_.print(indent_);
      needs_indent = true;
    }
  }
};

inline void ExprASTPrintVisitor::operator()(uptr<LetExprAST> &e) {
  std::printf("let ");
  bool needs_indent = false;
  for (auto &dec : e->decs) {
    if (needs_indent) {
      std::printf("\n");
      do_indent(indent_ + 4);
    }
    print(indent_ + 4, dec);
    needs_indent = true;
  }
  std::printf("\n");
  do_indent(indent_ + 1);
  std::printf("in ");
  std::visit(ExprASTPrintVisitor(indent_ + 4, true), e->body);
  std::printf("\n");
  do_indent(indent_);
  std::printf("end");
}

inline void print(int indent, VarAST &v) {
  std::visit(detail::VarASTPrintVisitor(indent), v);
}
inline void print(int indent, Ty &ty) {
  std::visit(detail::TyPrintVisitor(indent), ty);
}
inline void print(int indent, DeclAST &d) {
  std::visit(detail::DeclASTPrintVisitor(indent), d);
}

} // namespace detail

inline void FundecTy::print(int indent) {
  std::printf("function %s(", name.name());
  const char *sep = "";
  for (const auto &param : params) {
    std::printf("%s%s : %s", sep, param.name.name(), param.type_id.name());
    sep = ", ";
  }
  std::printf(")");
  if (result)
    std::printf(" : %s", result->sym.name());
  std::printf(" =\n");
  detail::do_indent(indent + 2);
  absyn::print(indent + 2, body);
}

inline void print(int indent, ExprAST &e) {
  std::visit(detail::ExprASTPrintVisitor(indent), e);
}

} // namespace absyn
#endif
