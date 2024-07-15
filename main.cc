#include "location.h"
#include "print.h"
#include "semant.h"
#include "token.h"
#include "tiger.tab.hh"
#include "types.h"
#include <cstring>
// defined in lex.yy.cc
int yylex_destroy();
// defined in tiger.tab.cc
extern uptr<absyn::ExprAST> parse_result;

int main() {
  yy::parser parser;
  parser();
  // since we're done with the input, we don't need the lexer any more
  yylex_destroy();
  if (parse_result) {
#ifdef PRINT_AST
    absyn::print(0, *parse_result);
    std::printf("\n");
#endif
    semant::Venv venv;
    semant::Tenv tenv;
    tenv.enter({symbol::Symbol(strdup("int")), types::IntTy()});
    tenv.enter({symbol::Symbol(strdup("string")), types::StringTy()});
    semant::trans_exp(venv, tenv, *parse_result);
  }
  symbol::Symbol::FreeAll();
}
