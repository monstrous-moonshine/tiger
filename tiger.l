/* vim: set ts=8 noet: */
%{
#include "token.h"
#include "tiger.tab.hh"
using token = yy::parser::token;
#define YY_DECL int yylex(Token *yylval)
#define YY_NO_UNPUT 1
%}

IDENT	[A-Za-z][A-Za-z0-9_]*
ESCAPE	\\(n|t|[0-9]{3}|\"|\\)
TSTRING	\"([[:print:]]{-}["\\]|{ESCAPE})*\"
USTRING	\"([[:print:]]{-}["\\]|{ESCAPE})*

%%
[ \t]*		/* ignore */
\n		yylval->line++;
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
[0-9]+		yylval->as.num = atoi(yytext); return token::INT;
{IDENT}		yylval->as.str = strndup(yytext, yyleng); return token::ID;
{TSTRING}	yylval->as.str = strndup(yytext, yyleng); return token::STR;
{USTRING}	fprintf(stderr, "Unterminated string on line %d\n", yylval->line); return token::YYerror;
"/*"		{
		char c = yyinput();
		int nest_level = 1;
		while (c != token::YYEOF && nest_level > 0) {
		    if (c == '\n') yylval->line++;
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
		    fprintf(stderr, "Unterminated comment on line %d\n", yylval->line);
		    return token::YYerror;
		}
		}
.		{
		fprintf(stderr, "Unexpected character %c on line %d\n", yytext[0], yylval->line);
		return token::YYerror;
		}

%%
int yywrap() {
    return 1;
}
