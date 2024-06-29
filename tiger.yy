/* vim: set ts=8 noet: */
%{
#include "token.h"
#include "absyn.h"

// defined in lex.yy.cc
int yylex(Token *yylval);
// defined below
ExprAST *expseq_to_expr(ExprSeq *);
// result of the parse
uptr<ExprAST> parse_result;
%}

%require "3.2"
%language "c++"
%define api.value.type {Token}

%token <as.str> ID
%token <as.num> INT
%token <as.str> STR
%token NIL

%nonassoc ASSIGN
%left '&' '|'
%nonassoc '=' NEQ '<' LE '>' GE
%left '+' '-'
%left '*' '/'

%token ARRAY BREAK DO ELSE END FOR FUNC IF IN LET NEW OF THEN TO TYPE VAR WHILE

%nterm <as.exp> exp op_exp assign_exp record_exp array_exp logical_exp term factor unary primary lvalue
%nterm <as.fields> fieldseq fields
%nterm <as.field> field
%nterm <as.exps> expseq exps argseq args
%nterm <as.decls> decs
%nterm <as.decl> dec vardec
%nterm <as.tydecs> tydecs
%nterm <as.fundecs> fundecs
%nterm <as.tydec> tydec
%nterm <as.ty> ty
%nterm <as.tyfields> tyfieldseq tyfields
%nterm <as.tyfield> tyfield
%nterm <as.fundec> fundec

%%
top:	exp				{ parse_result.reset($1); }
	;
exp:	op_exp
	|
	assign_exp
	|
	WHILE op_exp DO exp END		{ $$ = new WhileExprAST($2, $4); }
	|
	FOR ID ASSIGN op_exp TO op_exp DO exp END	{ $$ = new ForExprAST($2, $4, $6, $8); }
	|
	BREAK				{ $$ = new BreakExprAST(); }
	;
assign_exp:
	lvalue ASSIGN op_exp		{ $$ = new AssignExprAST($1, $3); }
	|
	lvalue ASSIGN record_exp	{ $$ = new AssignExprAST($1, $3); }
	|
	lvalue ASSIGN array_exp		{ $$ = new AssignExprAST($1, $3); }
	;
record_exp:
	NEW ID '{' fieldseq '}'		{ $$ = new RecordExprAST($2, $4); }
	;
array_exp:
	NEW ID '[' op_exp ']' OF op_exp { $$ = new ArrayExprAST($2, $4, $7); }
	;
lvalue: ID				{ $$ = new SimpleVarExprAST($1); }
	|
	lvalue '.' ID			{ $$ = new FieldVarExprAST($1, $3); }
	|
	lvalue '[' op_exp ']'		{ $$ = new IndexVarExprAST($1, $3); }
	;
op_exp:
	logical_exp
	|
	op_exp '&' logical_exp		{ $$ = new OpExprAST($1, $3, OP_AND); }
	|
	op_exp '|' logical_exp		{ $$ = new OpExprAST($1, $3, OP_OR); }
	;
logical_exp:
	term
	|
	logical_exp '=' term		{ $$ = new OpExprAST($1, $3, OP_EQ); }
	|
	logical_exp NEQ term		{ $$ = new OpExprAST($1, $3, OP_NEQ); }
	|
	logical_exp '<' term		{ $$ = new OpExprAST($1, $3, OP_LT); }
	|
	logical_exp LE	term		{ $$ = new OpExprAST($1, $3, OP_LE); }
	|
	logical_exp '>' term		{ $$ = new OpExprAST($1, $3, OP_GT); }
	|
	logical_exp GE	term		{ $$ = new OpExprAST($1, $3, OP_GE); }
	;
term:	factor
	|
	term '+' factor			{ $$ = new OpExprAST($1, $3, OP_PLUS); }
	|
	term '-' factor			{ $$ = new OpExprAST($1, $3, OP_MINUS); }
	;
factor: unary
	|
	factor '*' unary		{ $$ = new OpExprAST($1, $3, OP_MUL); }
	|
	factor '/' unary		{ $$ = new OpExprAST($1, $3, OP_DIV); }
	;
unary:	primary
	|
	'-' unary			{ $$ = new OpExprAST(new IntExprAST(0), $2, OP_MINUS); }
	;
primary:
	IF op_exp THEN exp ELSE exp END { $$ = new IfExprAST($2, $4, $6); }
	|
	IF op_exp THEN exp END		{ $$ = new IfExprAST($2, $4, nullptr); }
	|
	ID '(' argseq ')'		{ $$ = new CallExprAST($1, $3); }
	|
	'(' expseq ')'			{ $$ = expseq_to_expr($2); }
	|
	LET decs IN expseq END 		{ $$ = new LetExprAST($2, expseq_to_expr($4)); }
	|
	lvalue
	|
	INT				{ $$ = new IntExprAST($1); }
	|
	STR				{ $$ = new StringExprAST($1); }
	|
	NIL				{ $$ = new NilExprAST(); }
	;
expseq: /* empty */			{ $$ = new ExprSeq(); }
	|
	exps
	;
exps:	exp				{ $$ = new ExprSeq(); $$->AddExpr($1); }
	|
	exps ';' exp			{ $$ = $1; $$->AddExpr($3); }
	;
argseq: /* empty */			{ $$ = new ExprSeq(); }
	|
	args
	;
args:	exp				{ $$ = new ExprSeq(); $$->AddExpr($1); }
	|
	args ',' exp			{ $$ = $1; $$->AddExpr($3); }
	;
fieldseq:
	/* empty */			{ $$ = new FieldSeq(); }
	|
	fields
	;
fields: field				{ $$ = new FieldSeq(); $$->AddField($1); }
	|
	fields ',' field		{ $$ = $1; $$->AddField($3); }
	;
field:	ID '=' op_exp			{ $$ = new NamedExpr(Symbol($1), uptr<ExprAST>($3)); }
	;

/*============================== DECLARATIONS ==============================*/

decs: 	/* empty */ 			{ $$ = new DeclSeq(); }
	|
	decs dec 			{ $$ = $1; $$->AddDecl($2); }
	;
dec:	tydecs
	|
	vardec
	|
	fundecs
	;
tydecs: tydec 				{ $$ = new TypeDeclAST(); $$->AddType($1); }
	|
	tydecs tydec 			{ $$ = $1; $$->AddType($2); }
	;
tydec:	TYPE ID '=' ty 			{ $$ = new NamedType(Symbol($2), uptr<Ty>($4)); }
	;
ty:	ID 				{ $$ = new NameTy($1); }
	|
	'{' tyfieldseq '}' 		{ $$ = new RecordTy($2); }
	|
	ARRAY OF ID 			{ $$ = new ArrayTy($3); }
	;
tyfieldseq:
	/* empty */ 			{ $$ = new TyfieldSeq(); }
	|
	tyfields
	;
tyfields:
	tyfield 			{ $$ = new TyfieldSeq(); $$->AddField($1); }
	|
	tyfields ',' tyfield 		{ $$ = $1; $$->AddField($3);
	}
	;
tyfield:
	ID ':' ID 			{ $$ = new Tyfield($1, $3); }
	;
vardec:	VAR ID ASSIGN op_exp 		{ $$ = new VarDeclAST($2, nullptr, $4); }
	|
	VAR ID ':' ID ASSIGN op_exp 	{ $$ = new VarDeclAST($2, $4, $6); }
	;
fundecs:
	fundec 				{ $$ = new FuncDeclAST(); $$->AddFunc($1); }
	|
	fundecs fundec 			{ $$ = $1; $$->AddFunc($2);
	}
	;
fundec: FUNC ID '(' tyfieldseq ')' '=' op_exp
	{
	    $$ = new FundecTy($2, nullptr, $4, $7);
	}
	|
	FUNC ID '(' tyfieldseq ')' ':' ID '=' op_exp
	{
	    $$ = new FundecTy($2, $7, $4, $9);
	}
	;

%%
ExprAST *expseq_to_expr(ExprSeq *exps) {
    switch (exps->seq.size()) {
    case 0:
	delete exps;
	return nullptr;
    case 1: {
	auto *ptr = exps->seq[0].release();
	delete exps;
	return ptr;
    }
    default:
	return new SeqExprAST(exps);
    }
}

namespace yy {
    void parser::error(const std::string& msg) {
	std::cerr << msg << "\n";
    }
}
