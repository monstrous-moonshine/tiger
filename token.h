#ifndef TOKEN_H
#define TOKEN_H
#include <memory>
#include <utility>

namespace symbol {
class Symbol;
}

namespace absyn {

class ExprAST;
class VarAST;
// sequence of expressions in (..; ..; ..) and let expression
class ExprSeq;
// all "name = exp" pairs in a record expression
class FieldSeq;
// one "name = exp" pair in a record expression
using Field = std::pair<symbol::Symbol, std::unique_ptr<ExprAST>>;

class DeclAST;
class TypeDeclAST;
class FuncDeclAST;
// sequence of declarations in a let expression
class DeclSeq;
// type alias, record, or array type
class Ty;
// "name = id | record | array" pair
// one type declaration in a mutually recursive set
using Type = std::pair<symbol::Symbol, std::unique_ptr<Ty>>;
// all "name: type" pairs in a record declaration
class FieldTySeq;
// one "name: type" pair in a record declaration
class FieldTy;
// one function declaration in a mutually recursive set
class FundecTy;

} // namespace absyn

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
    absyn::TypeDeclAST *tydecs;
    absyn::FuncDeclAST *fundecs;
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
