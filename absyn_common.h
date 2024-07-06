#ifndef ABSYN_COMMON_H
#define ABSYN_COMMON_H
#include <memory>
#include <variant>
template <typename T> using uptr = std::unique_ptr<T>;

namespace symbol {
class Symbol;
}

namespace absyn {

struct SimpleVarAST;
struct FieldVarAST;
struct IndexVarAST;
using VarAST =
    std::variant<uptr<SimpleVarAST>, uptr<FieldVarAST>, uptr<IndexVarAST>>;

struct VarExprAST;
struct NilExprAST;
struct IntExprAST;
struct StringExprAST;
struct CallExprAST;
struct OpExprAST;
struct RecordExprAST;
struct ArrayExprAST;
struct SeqExprAST;
struct AssignExprAST;
struct IfExprAST;
struct WhileExprAST;
struct ForExprAST;
struct BreakExprAST;
struct LetExprAST;

using ExprAST =
    std::variant<uptr<VarExprAST>, uptr<NilExprAST>, uptr<IntExprAST>,
                 uptr<StringExprAST>, uptr<CallExprAST>, uptr<OpExprAST>,
                 uptr<RecordExprAST>, uptr<ArrayExprAST>, uptr<SeqExprAST>,
                 uptr<AssignExprAST>, uptr<IfExprAST>, uptr<WhileExprAST>,
                 uptr<ForExprAST>, uptr<BreakExprAST>, uptr<LetExprAST>>;

struct NameTy;
struct RecordTy;
struct ArrayTy;
using Ty = std::variant<uptr<NameTy>, uptr<RecordTy>, uptr<ArrayTy>>;

struct TypeDeclAST;
struct VarDeclAST;
struct FuncDeclAST;
using DeclAST =
    std::variant<uptr<TypeDeclAST>, uptr<VarDeclAST>, uptr<FuncDeclAST>>;

using Field = std::pair<symbol::Symbol, ExprAST>;
using Type = std::pair<symbol::Symbol, Ty>;

class ExprSeq;
class FieldSeq;
class DeclSeq;
class FieldTySeq;
class FieldTy;
class FundecTy;

} // namespace absyn
#endif
