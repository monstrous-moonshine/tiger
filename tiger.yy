/* vim: set ts=8 noet: */
%{
#include "token.h"
#include "absyn.h"

// defined in lex.yy.cc
int yylex(Token *yylval);

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

%nterm <as.exp> exp op_exp assign_exp record_exp array_exp logical_exp term factor unary primary
%nterm <as.var> lvalue
%nterm <as.fields> fieldseq fields
%nterm <as.field> field
%nterm <as.exps> expseq exps argseq args
%nterm <as.decls> decs
%nterm <as.decl> dec vardec tydecs fundecs
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
	IF exp THEN exp			{ $$ = new E(IfExprAST, $2, $4, nullptr); }
	|
	IF exp THEN exp ELSE exp	{ $$ = new E(IfExprAST, $2, $4, $6); }
	|
	WHILE exp DO exp		{ $$ = new E(WhileExprAST, $2, $4); }
	|
	FOR ID ASSIGN exp TO exp DO exp	{ $$ = new E(ForExprAST, $2, $4, $6, $8); }
	|
	LET decs IN expseq END		{ $$ = new E(LetExprAST, $2, expseq_to_expr($4)); }
	|
	record_exp
	|
	array_exp
	|
	BREAK				{ $$ = new E(BreakExprAST, ); }
	;
record_exp:
	NEW ID '{' fieldseq '}'		{ $$ = new E(RecordExprAST, $2, $4); }
	;
array_exp:
	NEW ID '[' op_exp ']' OF op_exp	{ $$ = new E(ArrayExprAST, $2, $4, $7); }
	;
assign_exp:
	lvalue ASSIGN exp		{ $$ = new E(AssignExprAST, $1, $3); }
	;
lvalue: ID				{ $$ = new V(SimpleVarAST, $1); }
	|
	lvalue '.' ID			{ $$ = new V(FieldVarAST, $1, $3); }
	|
	lvalue '[' exp ']'		{ $$ = new V(IndexVarAST, $1, $3); }
	;
op_exp:
	logical_exp
	|
	op_exp '&' logical_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kAnd); }
	|
	op_exp '|' logical_exp		{ $$ = new E(OpExprAST, $1, $3, Op::kOr); }
	;
logical_exp:
	term
	|
	logical_exp '=' term		{ $$ = new E(OpExprAST, $1, $3, Op::kEq); }
	|
	logical_exp NEQ term		{ $$ = new E(OpExprAST, $1, $3, Op::kNeq); }
	|
	logical_exp '<' term		{ $$ = new E(OpExprAST, $1, $3, Op::kLt); }
	|
	logical_exp LE	term		{ $$ = new E(OpExprAST, $1, $3, Op::kLe); }
	|
	logical_exp '>' term		{ $$ = new E(OpExprAST, $1, $3, Op::kGt); }
	|
	logical_exp GE	term		{ $$ = new E(OpExprAST, $1, $3, Op::kGe); }
	;
term:	factor
	|
	term '+' factor			{ $$ = new E(OpExprAST, $1, $3, Op::kPlus); }
	|
	term '-' factor			{ $$ = new E(OpExprAST, $1, $3, Op::kMinus); }
	;
factor: unary
	|
	factor '*' unary		{ $$ = new E(OpExprAST, $1, $3, Op::kMul); }
	|
	factor '/' unary		{ $$ = new E(OpExprAST, $1, $3, Op::kDiv); }
	;
unary:	primary
	|
	'-' unary			{ $$ = new E(OpExprAST, new IntExprAST(0), $2, Op::kMinus); }
	;
primary:
	ID '(' argseq ')'		{ $$ = new E(CallExprAST, $1, $3); }
	|
	'(' expseq ')'			{ $$ = expseq_to_expr($2); }
	|
	lvalue				{ $$ = new E(VarExprAST, $1); }
	|
	INT				{ $$ = new E(IntExprAST, $1); }
	|
	STR				{ $$ = new E(StringExprAST, $1); }
	|
	NIL				{ $$ = new E(NilExprAST, ); }
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
field:	ID '=' op_exp			{ $$ = new Field(Symbol($1), std::move(*$3)); }
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
tydecs: tydec 				{ $$ = new D(TypeDeclAST); TD(*$$)->AddType($1); }
	|
	tydecs tydec 			{ $$ = $1; TD(*$$)->AddType($2); }
	;
tydec:	TYPE ID '=' ty 			{ $$ = new Type(Symbol($2), std::move(*$4)); }
	;
ty:	ID 				{ $$ = new TV(NameTy, $1); }
	|
	'{' tyfieldseq '}' 		{ $$ = new TV(RecordTy, $2); }
	|
	ARRAY OF ID 			{ $$ = new TV(ArrayTy, $3); }
	;
tyfieldseq:
	/* empty */ 			{ $$ = new FieldTySeq(); }
	|
	tyfields
	;
tyfields:
	tyfield 			{ $$ = new FieldTySeq(); $$->AddField($1); }
	|
	tyfields ',' tyfield 		{ $$ = $1; $$->AddField($3);
	}
	;
tyfield:
	ID ':' ID 			{ $$ = new FieldTy($1, $3); }
	;
vardec:	VAR ID ASSIGN exp 		{ $$ = new D(VarDeclAST, $2, nullptr, $4); }
	|
	VAR ID ':' ID ASSIGN exp 	{ $$ = new D(VarDeclAST, $2, $4, $6); }
	;
fundecs:
	fundec 				{ $$ = new D(FuncDeclAST, ); FD(*$$)->AddFunc($1); }
	|
	fundecs fundec 			{ $$ = $1; FD(*$$)->AddFunc($2);
	}
	;
fundec: FUNC ID '(' tyfieldseq ')' '=' exp
	{
	    $$ = new FundecTy($2, $4, nullptr, $7);
	}
	|
	FUNC ID '(' tyfieldseq ')' ':' ID '=' exp
	{
	    $$ = new FundecTy($2, $4, $7, $9);
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
	auto exp = new ExprAST{std::move(exps->seq[0])};
	delete exps;
	return exp;
    }
    default:
	return new E(SeqExprAST, exps);
    }
}

void parser::error(const std::string& msg) {
    std::cerr << msg << "\n";
}

} // namespace
