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
using std::make_unique;
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
	IF op_exp THEN exp END		{ $$ = new ExprAST{make_unique<IfExprAST>($2, $4, nullptr)}; }
	|
	WHILE op_exp DO exp END		{ $$ = new ExprAST{make_unique<WhileExprAST>($2, $4)}; }
	|
	FOR ID ASSIGN op_exp TO op_exp DO exp END	{ $$ = new ExprAST{make_unique<ForExprAST>($2, $4, $6, $8)}; }
	|
	BREAK				{ $$ = new ExprAST{make_unique<BreakExprAST>()}; }
	;
assign_exp:
	lvalue ASSIGN op_exp		{ $$ = new ExprAST{make_unique<AssignExprAST>($1, $3)}; }
	;
lvalue: ID				{ $$ = new VarAST{make_unique<SimpleVarAST>($1)}; }
	|
	lvalue '.' ID			{ $$ = new VarAST{make_unique<FieldVarAST>($1, $3)}; }
	|
	lvalue '[' op_exp ']'		{ $$ = new VarAST{make_unique<IndexVarAST>($1, $3)}; }
	;
op_exp:
	logical_exp
	|
	op_exp '&' logical_exp		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kAnd)}; }
	|
	op_exp '|' logical_exp		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kOr)}; }
	;
logical_exp:
	term
	|
	logical_exp '=' term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kEq)}; }
	|
	logical_exp NEQ term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kNeq)}; }
	|
	logical_exp '<' term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kLt)}; }
	|
	logical_exp LE	term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kLe)}; }
	|
	logical_exp '>' term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kGt)}; }
	|
	logical_exp GE	term		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kGe)}; }
	;
term:	factor
	|
	term '+' factor			{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kPlus)}; }
	|
	term '-' factor			{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kMinus)}; }
	;
factor: unary
	|
	factor '*' unary		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kMul)}; }
	|
	factor '/' unary		{ $$ = new ExprAST{make_unique<OpExprAST>($1, $3, Op::kDiv)}; }
	;
unary:	primary
	|
	'-' unary			{ $$ = new ExprAST{make_unique<OpExprAST>(new IntExprAST(0), $2, Op::kMinus)}; }
	;
primary:
	IF op_exp THEN exp ELSE exp END { $$ = new ExprAST{make_unique<IfExprAST>($2, $4, $6)}; }
	|
	ID '(' argseq ')'		{ $$ = new ExprAST{make_unique<CallExprAST>($1, $3)}; }
	|
	'(' expseq ')'			{ $$ = expseq_to_expr($2); }
	|
	LET decs IN expseq END 		{ $$ = new ExprAST{make_unique<LetExprAST>($2, expseq_to_expr($4))}; }
	|
	lvalue				{ $$ = new ExprAST{make_unique<VarExprAST>($1)}; }
	|
	record_exp
	|
	array_exp
	|
	INT				{ $$ = new ExprAST{make_unique<IntExprAST>($1)}; }
	|
	STR				{ $$ = new ExprAST{make_unique<StringExprAST>($1)}; }
	|
	NIL				{ $$ = new ExprAST{make_unique<NilExprAST>()}; }
	;
record_exp:
	NEW ID '{' fieldseq '}'		{ $$ = new ExprAST{make_unique<RecordExprAST>($2, $4)}; }
	;
array_exp:
	NEW ID '[' op_exp ']' OF op_exp END	{ $$ = new ExprAST{make_unique<ArrayExprAST>($2, $4, $7)}; }
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
field:	ID '=' op_exp			{ $$ = new Field(Symbol($1), uptr<ExprAST>($3)); }
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
tydecs: tydec 				{ $$ = new DeclAST{make_unique<TypeDeclAST>()}; std::get<uptr<TypeDeclAST>>(*$$)->AddType($1); }
	|
	tydecs tydec 			{ $$ = $1; std::get<uptr<TypeDeclAST>>(*$$)->AddType($2); }
	;
tydec:	TYPE ID '=' ty 			{ $$ = new Type(Symbol($2), std::move(*$4)); }
	;
ty:	ID 				{ $$ = new Ty{make_unique<NameTy>($1)}; }
	|
	'{' tyfieldseq '}' 		{ $$ = new Ty{make_unique<RecordTy>($2)}; }
	|
	ARRAY OF ID 			{ $$ = new Ty{make_unique<ArrayTy>($3)}; }
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
vardec:	VAR ID ASSIGN op_exp 		{ $$ = new DeclAST{make_unique<VarDeclAST>($2, nullptr, $4)}; }
	|
	VAR ID ':' ID ASSIGN op_exp 	{ $$ = new DeclAST{make_unique<VarDeclAST>($2, $4, $6)}; }
	;
fundecs:
	fundec 				{ $$ = new DeclAST{make_unique<FuncDeclAST>()}; std::get<uptr<FuncDeclAST>>(*$$)->AddFunc($1); }
	|
	fundecs fundec 			{ $$ = $1; std::get<uptr<FuncDeclAST>>(*$$)->AddFunc($2);
	}
	;
fundec: FUNC ID '(' tyfieldseq ')' '=' op_exp
	{
	    $$ = new FundecTy($2, $4, nullptr, $7);
	}
	|
	FUNC ID '(' tyfieldseq ')' ':' ID '=' op_exp
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
	return nullptr;
    case 1: {
	auto *ptr = exps->seq[0].release();
	delete exps;
	return new ExprAST(std::move(*ptr));
    }
    default:
	return new ExprAST{make_unique<SeqExprAST>(exps)};
    }
}

void parser::error(const std::string& msg) {
    std::cerr << msg << "\n";
}

} // namespace
