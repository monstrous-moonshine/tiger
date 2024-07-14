/* vim: set ts=8 noet: */
%{
#include "token.h"
#include "absyn.h"
#include "location.h"

// defined in lex.yy.cc
int yylex(Token *yylval, Location *yylloc);

using namespace absyn;
// result of the parse
uptr<ExprAST> parse_result;

namespace yy {
ExprAST *expseq_to_expr(ExprSeq *);
}

#define E(type, ...) ExprAST{std::make_unique<type>(__VA_ARGS__)}
#define V(type, ...) VarAST{std::make_unique<type>(__VA_ARGS__)}
#define D(type, ...) DeclAST{std::make_unique<type>(__VA_ARGS__)}
#define TV(type, ...) Ty{std::make_unique<type>(__VA_ARGS__)}
#define TD(val) std::get<uptr<TypeDeclAST>>(val)
#define FD(val) std::get<uptr<FuncDeclAST>>(val)

#define YYLLOC_DEFAULT(Current, Rhs, N)	 \
    do					 \
      if (N)				 \
	{				 \
	  (Current) = YYRHSLOC (Rhs, 1); \
	}				 \
      else				 \
	{				 \
	  (Current) = YYRHSLOC (Rhs, 0); \
	}				 \
    while (false)
%}

%require "3.2"
%language "c++"
%locations
%define api.location.type {Location}
%define api.value.type {Token}

%token <str> ID
%token <num> INT
%token <str> STR
%token NIL

%nonassoc ASSIGN
%left '&' '|'
%nonassoc '=' NEQ '<' LE '>' GE
%left '+' '-'
%left '*' '/'
%left UMINUS

%token ARRAY BREAK DO ELSE END FOR FUNC IF IN LET NEW OF THEN TO TYPE VAR WHILE

%nterm <exp> exp op_exp primary
%nterm <var> lvalue
%nterm <fields> fieldseq fields
%nterm <field> field
%nterm <exps> expseq exps argseq args
%nterm <decls> decs
%nterm <decl> dec vardec tydecs fundecs
%nterm <tydec> tydec
%nterm <fundec> fundec
%nterm <ty> ty
%nterm <tyfields> tyfieldseq tyfields
%nterm <tyfield> tyfield

%%
prog:	exp				{ parse_result.reset($1); }
	;
exp:	op_exp
	|
	lvalue ASSIGN exp		{ $$ = new E(AssignExprAST, $1, $3, @2); }
	|
	IF exp THEN exp			{ $$ = new E(IfExprAST, $2, $4, nullptr, @1); }
	|
	IF exp THEN exp ELSE exp	{ $$ = new E(IfExprAST, $2, $4, $6, @1); }
	|
	WHILE exp DO exp		{ $$ = new E(WhileExprAST, $2, $4, @1); }
	|
	FOR ID ASSIGN exp TO exp DO exp	{ $$ = new E(ForExprAST, $2, $4, $6, $8, @1); }
	|
	LET decs IN expseq END		{ $$ = new E(LetExprAST, $2, expseq_to_expr($4), @1); }
	|
	NEW ID '{' fieldseq '}'		{ $$ = new E(RecordExprAST, $2, $4, @2); }
	|
	NEW ID '[' exp ']' OF exp	{ $$ = new E(ArrayExprAST, $2, $4, $7, @2); }
	|
	BREAK				{ $$ = new E(BreakExprAST, @1); }
	;
lvalue: ID				{ $$ = new V(SimpleVarAST, $1, @1); }
	|
	lvalue '.' ID			{ $$ = new V(FieldVarAST, $1, $3, @2); }
	|
	lvalue '[' exp ']'		{ $$ = new V(IndexVarAST, $1, $3, @2); }
	;
op_exp: op_exp '&' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kAnd, @2); }
	|
	op_exp '|' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kOr, @2); }
	|
	op_exp '=' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kEq, @2); }
	|
	op_exp NEQ op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kNeq, @2); }
	|
	op_exp '<' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kLt, @2); }
	|
	op_exp LE op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kLe, @2); }
	|
	op_exp '>' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kGt, @2); }
	|
	op_exp GE op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kGe, @2); }
	|
	op_exp '+' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kPlus, @2); }
	|
	op_exp '-' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kMinus, @2); }
	|
	op_exp '*' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kMul, @2); }
	|
	op_exp '/' op_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kDiv, @2); }
	|
	'-' op_exp %prec UMINUS		{ $$ = new E(OpExprAST, new IntExprAST(0), $2, Op::kMinus, @1); }
	|
	primary
	;
primary:
	ID '(' argseq ')'		{ $$ = new E(CallExprAST, $1, $3, @2); }
	|
	'(' expseq ')'			{ $$ = expseq_to_expr($2); }
	|
	lvalue				{ $$ = new E(VarExprAST, $1); }
	|
	INT				{ $$ = new E(IntExprAST, $1); }
	|
	STR				{ $$ = new E(StringExprAST, $1); }
	|
	NIL				{ $$ = new E(NilExprAST); }
	;
expseq: /* empty */			{ $$ = new ExprSeq(); }
	|
	exps
	;
exps:	exp				{ $$ = new ExprSeq(); $$->AddExpr($1, @1); }
	|
	exps ';' exp			{ $$ = $1; $$->AddExpr($3, @3); }
	;
argseq: /* empty */			{ $$ = new ExprSeq(); }
	|
	args
	;
args:	exp				{ $$ = new ExprSeq(); $$->AddExpr($1, @1); }
	|
	args ',' exp			{ $$ = $1; $$->AddExpr($3, @3); }
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
field:	ID '=' op_exp			{ $$ = new Field($1, $3, @1); }
	;

/*============================== DECLARATIONS ==============================*/

decs:	/* empty */			{ $$ = new DeclSeq(); }
	|
	decs dec			{ $$ = $1; $$->AddDecl($2); }
	;
dec:	tydecs
	|
	vardec
	|
	fundecs
	;
tydecs: tydec				{ $$ = new D(TypeDeclAST); TD(*$$)->AddType($1); }
	|
	tydecs tydec			{ $$ = $1; TD(*$$)->AddType($2); }
	;
tydec:	TYPE ID '=' ty			{ $$ = new Type($2, $4, @1); }
	;
ty:	ID				{ $$ = new TV(NameTy, $1, @1); }
	|
	'{' tyfieldseq '}'		{ $$ = new TV(RecordTy, $2); }
	|
	ARRAY OF ID			{ $$ = new TV(ArrayTy, $3, @3); }
	;
tyfieldseq:
	/* empty */			{ $$ = new FieldTySeq(); }
	|
	tyfields
	;
tyfields:
	tyfield				{ $$ = new FieldTySeq(); $$->AddField($1); }
	|
	tyfields ',' tyfield		{ $$ = $1; $$->AddField($3);
	}
	;
tyfield:
	ID ':' ID			{ $$ = new FieldTy($1, $3, @1); }
	;
vardec:	VAR ID ASSIGN exp		{ $$ = new D(VarDeclAST, $2, nullptr, @1, $4, @1); }
	|
	VAR ID ':' ID ASSIGN exp	{ $$ = new D(VarDeclAST, $2, $4, @4, $6, @1); }
	;
fundecs:
	fundec				{ $$ = new D(FuncDeclAST); FD(*$$)->AddFunc($1); }
	|
	fundecs fundec			{ $$ = $1; FD(*$$)->AddFunc($2);
	}
	;
fundec: FUNC ID '(' tyfieldseq ')' '=' exp
	{
	    $$ = new FundecTy($2, $4, nullptr, @1, $7, @1);
	}
	|
	FUNC ID '(' tyfieldseq ')' ':' ID '=' exp
	{
	    $$ = new FundecTy($2, $4, $7, @7, $9, @1);
	}
	;

%%
namespace yy {

ExprAST *expseq_to_expr(ExprSeq *exps) {
    switch (exps->seq.size()) {
    case 0:
	delete exps;
	return new E(UnitExprAST);
    case 1: {
	auto exp = new ExprAST{std::move(exps->seq[0].exp)};
	delete exps;
	return exp;
    }
    default:
	return new E(SeqExprAST, exps);
    }
}

void parser::error(const Location& loc, const std::string& msg) {
    std::printf("%s at %d:%d\n", msg.c_str(), loc.line, loc.column);
}

} // namespace
