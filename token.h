#ifndef TOKEN_H
#define TOKEN_H
#include "absyn_common.h"

struct Token {
  union {
    int num;
    char *str;
    absyn::ExprAST *exp;
    absyn::VarAST *var;
    absyn::ExprSeq *exps;
    absyn::FieldSeq *fields;
    absyn::Field *field;
    absyn::DeclAST *decl;
    absyn::DeclSeq *decls;
    absyn::Ty *ty;
    absyn::Type *tydec;
    absyn::FieldTySeq *tyfields;
    absyn::FieldTy *tyfield;
    absyn::FundecTy *fundec;
  } as;
  int line;
};
#endif
