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
  symbol::Symbol sym_int(strdup("int"));
  symbol::Symbol sym_str(strdup("string"));
  yy::parser parser;
  parser();
  // since we're done with the input, we don't need the lexer any more
  yylex_destroy();
  if (parse_result) {
    absyn::print(0, *parse_result);
    std::printf("\n");
    semant::Venv venv;
    semant::Tenv tenv;
    tenv.enter({sym_int, types::IntTy()});
    tenv.enter({sym_str, types::StringTy()});
    semant::trans_exp(venv, tenv, *parse_result);
  }
  symbol::Symbol::FreeAll();
}
