#include "token.h"
#include "absyn.h"
#include "tiger.tab.hh"
#include "print.h"
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
    absyn::ExprASTPrintVisitor v(0);
    parse_result->accept(v);
  }
  // since we've printed all the symbols, their strings can be freed
  symbol::Symbol::FreeAll();
}
