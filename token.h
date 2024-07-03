#ifndef TOKEN_H
#define TOKEN_H
#include <memory>
#include <utility>

namespace symbol {
class Symbol;
}

namespace absyn {

class ExprAST;
// sequence of expressions in (..; ..; ..) and let expression
class ExprSeq;
// all "name = exp" pairs in a record expression
class FieldSeq;
// one "name = exp" pair in a record expression
using NamedExpr = std::pair<symbol::Symbol, std::unique_ptr<ExprAST>>;

class DeclAST;
class TypeDeclAST;
class FuncDeclAST;
// sequence of declarations in a let expression
class DeclSeq;
// type alias, record, or array type
class Ty;
// "name = id | record | array" pair
// one type declaration in a mutually recursive set
using NamedType = std::pair<symbol::Symbol, std::unique_ptr<Ty>>;
// all "name: type" pairs in a record declaration
class TyfieldSeq;
// one "name: type" pair in a record declaration
class Tyfield;
// one function declaration in a mutually recursive set
class FundecTy;

} // namespace absyn

struct Token {
  union {
    int num;
    char *str;
    absyn::ExprAST *exp;
    absyn::ExprSeq *exps;
    absyn::FieldSeq *fields;
    absyn::NamedExpr *field;
    absyn::DeclAST *decl;
    absyn::TypeDeclAST *tydecs;
    absyn::FuncDeclAST *fundecs;
    absyn::DeclSeq *decls;
    absyn::Ty *ty;
    absyn::NamedType *tydec;
    absyn::TyfieldSeq *tyfields;
    absyn::Tyfield *tyfield;
    absyn::FundecTy *fundec;
  } as;
  int line;
};
#endif
