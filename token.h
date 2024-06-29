#ifndef TOKEN_H
#define TOKEN_H
#include <memory>
#include <utility>
class Symbol;

class ExprAST;
// sequence of expressions in (..; ..; ..) and let expression
class ExprSeq;
// all "name = exp" pairs in a record expression
class FieldSeq;
// one "name = exp" pair in a record expression
using NamedExpr = std::pair<Symbol, std::unique_ptr<ExprAST>>;

class DeclAST;
class TypeDeclAST;
class FuncDeclAST;
// sequence of declarations in a let expression
class DeclSeq;
// type alias, record, or array type
class Ty;
// "name = id | record | array" pair
// one type declaration in a mutually recursive set
using NamedType = std::pair<Symbol, std::unique_ptr<Ty>>;
// all "name: type" pairs in a record declaration
class TyfieldSeq;
// one "name: type" pair in a record declaration
class Tyfield;
// one function declaration in a mutually recursive set
class FundecTy;

struct Token {
  union {
    int num;
    char *str;
    ExprAST *exp;
    ExprSeq *exps;
    FieldSeq *fields;
    NamedExpr *field;
    DeclAST *decl;
    TypeDeclAST *tydecs;
    FuncDeclAST *fundecs;
    DeclSeq *decls;
    Ty *ty;
    NamedType *tydec;
    TyfieldSeq *tyfields;
    Tyfield *tyfield;
    FundecTy *fundec;
  } as;
  int line;
};
#endif
