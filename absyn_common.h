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
struct UnitExprAST;

using ExprAST = std::variant<
    uptr<VarExprAST>, uptr<NilExprAST>, uptr<IntExprAST>, uptr<StringExprAST>,
    uptr<CallExprAST>, uptr<OpExprAST>, uptr<RecordExprAST>, uptr<ArrayExprAST>,
    uptr<SeqExprAST>, uptr<AssignExprAST>, uptr<IfExprAST>, uptr<WhileExprAST>,
    uptr<ForExprAST>, uptr<BreakExprAST>, uptr<LetExprAST>, uptr<UnitExprAST>>;

struct NameTy;
struct RecordTy;
struct ArrayTy;
using Ty = std::variant<uptr<NameTy>, uptr<RecordTy>, uptr<ArrayTy>>;

struct TypeDeclAST;
struct VarDeclAST;
struct FuncDeclAST;
using DeclAST =
    std::variant<uptr<TypeDeclAST>, uptr<VarDeclAST>, uptr<FuncDeclAST>>;

struct RExprField;
struct RTyField;
struct Type;
struct FundecTy;

class ExprSeq;
class DeclSeq;
class RExprFieldSeq;
class RTyFieldSeq;

} // namespace absyn
#endif
