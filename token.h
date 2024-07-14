#ifndef TOKEN_H
#define TOKEN_H
#include "absyn_common.h"

union Token {
  int num;
  char *str;
  absyn::ExprAST *exp;
  absyn::VarAST *var;
  absyn::ExprSeq *exps;
  absyn::RExprFieldSeq *fields;
  absyn::RExprField *field;
  absyn::DeclAST *decl;
  absyn::DeclSeq *decls;
  absyn::Ty *ty;
  absyn::Type *tydec;
  absyn::RTyFieldSeq *tyfields;
  absyn::RTyField *tyfield;
  absyn::FundecTy *fundec;
};
#endif
