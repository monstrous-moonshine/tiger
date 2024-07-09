/* vim: set ts=8 noet: */
%{
#include "token.h"
#include "location.h"
#include "tiger.tab.hh"
using token = yy::parser::token;
namespace {
  int line_ = 1, column_ = 1;
  void line() {
    line_++;
    column_ = 1;
  }
}
#define YY_DECL int yylex(Token *yylval, Location *yylloc)
#define YY_NO_UNPUT 1
#define YY_USER_ACTION *yylloc = {line_, column_}; column_ += yyleng;
%}

IDENT	[A-Za-z][A-Za-z0-9_]*
ESCAPE	\\(n|t|[0-9]{3}|\"|\\)
TSTRING	\"([[:print:]]{-}["\\]|{ESCAPE})*\"
USTRING	\"([[:print:]]{-}["\\]|{ESCAPE})*

%%
[ \t]*		/* ignore */
\n		line();
\{|\}|:|,|\(|\)|\.|\[|\]|;|\+|-|\*|\/|=|<|>|&|\|	return yytext[0];
":="		return token::ASSIGN;
"<>"		return token::NEQ;
"<="		return token::LE;
">="		return token::GE;
array		return token::ARRAY;
break		return token::BREAK;
do		return token::DO;
else		return token::ELSE;
end		return token::END;
for		return token::FOR;
function	return token::FUNC;
if		return token::IF;
in		return token::IN;
let		return token::LET;
new		return token::NEW;
nil		return token::NIL;
of		return token::OF;
then		return token::THEN;
to		return token::TO;
type		return token::TYPE;
var		return token::VAR;
while		return token::WHILE;
[0-9]+		yylval->num = atoi(yytext); return token::INT;
{IDENT}		yylval->str = strndup(yytext, yyleng); return token::ID;
{TSTRING}	yylval->str = strndup(yytext, yyleng); return token::STR;
{USTRING}	fprintf(stderr, "Unterminated string on line %d\n", line_); return token::YYerror;
"/*"		{
		char c = yyinput();
		int nest_level = 1;
		while (c != token::YYEOF && nest_level > 0) {
		    if (c == '\n') line();
		    if (c == '*') {
			c = yyinput();
			if (c == '/') { nest_level--; c = yyinput(); }
		    } else if (c == '/') {
			c = yyinput();
			if (c == '*') { nest_level++; c = yyinput(); }
		    } else {
			c = yyinput();
		    }
		}
		if (c == token::YYEOF) {
		    fprintf(stderr, "Unterminated comment on line %d\n", line_);
		    return token::YYerror;
		}
		}
.		{
		fprintf(stderr, "Unexpected character %c on line %d\n", yytext[0], line_);
		return token::YYerror;
		}

%%
int yywrap() {
    return 1;
}
