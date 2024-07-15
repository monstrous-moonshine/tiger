#include "semant.h"
#include "absyn.h"
#include "absyn_common.h"
#include "box.h"
#include "env.h"
#include "location.h"
#include "logging.h"
#include "types.h"
#include <algorithm>
#include <unordered_set>
#include <variant>

namespace {
class LoopManager {
public:
  using Entry = std::variant<absyn::ForExprAST *, absyn::WhileExprAST *>;
  static LoopManager &Get() {
    static LoopManager g;
    return g;
  }
  void EnterFun() { loops_.push_front(std::forward_list<Entry>{}); }
  void ExitFun() { loops_.pop_front(); }
  void EnterLoop(absyn::ForExprAST *e) { loops_.front().push_front(e); }
  void EnterLoop(absyn::WhileExprAST *e) { loops_.front().push_front(e); }
  void ExitLoop() { loops_.front().pop_front(); }
  bool IsLoop() const { return !loops_.empty() && !loops_.front().empty(); }

private:
  std::forward_list<std::forward_list<Entry>> loops_;
  LoopManager() : loops_(1) {}
};
} // namespace

namespace semant {

bool is_int(const Expty &et) { return types::is<types::IntTy>(et.ty); }
bool is_str(const Expty &et) { return types::is<types::StringTy>(et.ty); }
bool is_record(const Expty &et) { return types::is<types::RecordTyRef>(et.ty); }
bool is_array(const Expty &et) { return types::is<types::ArrayTyRef>(et.ty); }
bool is_nil(const Expty &et) { return types::is<types::NilTy>(et.ty); }

void trans_dec(Venv &, Tenv &, absyn::DeclAST &);
types::Ty trans_ty(Tenv &, absyn::Ty &);

class TransExp {
  Venv &venv;
  Tenv &tenv;
  class ExprVisitor {
    TransExp &e_;

  public:
    ExprVisitor(TransExp &enclosing) : e_(enclosing) {}
    Expty operator()(uptr<absyn::VarExprAST> &e) { return e_.trvar(e->var); }
    Expty operator()(uptr<absyn::NilExprAST> &e) { return {types::NilTy()}; }
    Expty operator()(uptr<absyn::IntExprAST> &e) { return {types::IntTy()}; }
    Expty operator()(uptr<absyn::StringExprAST> &e) {
      return {types::StringTy()};
    }
    Expty operator()(uptr<absyn::CallExprAST> &e) {
      auto entry = e_.venv.look(e->func);
      CHECK(entry) << e->pos << ": Undefined symbol '" << e->func.name() << "'";
      CHECK(env::is<env::FunEntry>(entry.value()))
          << e->pos << ": '" << e->func.name() << "' is not a function";
      auto &func = env::as<env::FunEntry>(entry.value());
      CHECK_EQ(e->args.size(), func.formals.size()) << e->pos;
      for (int i = 0; i < (int)e->args.size(); i++) {
        auto et = e_.trexp(e->args[i].exp);
        CHECK(types::is_compatible(et.ty, func.formals[i])) << e->args[i].pos;
      }
      return {func.result};
    }
    Expty operator()(uptr<absyn::OpExprAST> &e) {
      auto lhs = e_.trexp(e->lhs);
      auto rhs = e_.trexp(e->rhs);
      switch (e->op) {
      case absyn::Op::kEq:
      case absyn::Op::kNeq:
        CHECK(is_int(lhs) || is_str(lhs) || is_record(lhs) || is_array(lhs) ||
              is_nil(lhs))
            << e->pos;
        CHECK(types::is_compatible(lhs.ty, rhs.ty)) << e->pos;
        break;
      case absyn::Op::kLt:
      case absyn::Op::kGt:
      case absyn::Op::kLe:
      case absyn::Op::kGe:
        CHECK(is_int(lhs) || is_str(lhs)) << e->pos;
        CHECK(types::is_compatible(lhs.ty, rhs.ty)) << e->pos;
        break;
      default:
        CHECK(is_int(lhs)) << e->pos;
        CHECK(is_int(rhs)) << e->pos;
      }
      return {types::IntTy()};
    }
    Expty operator()(uptr<absyn::RecordExprAST> &e) {
      auto entry = e_.tenv.look(e->type_id);
      CHECK(entry) << e->pos << ": Undefined symbol '" << e->type_id.name()
                   << "'";
      CHECK(types::is<types::RecordTyRef>(entry.value()))
          << e->pos << ": '" << e->type_id.name() << "' is not a record";
      auto &ty = types::as<types::RecordTyRef>(entry.value());
      CHECK_EQ(e->fields.size(), ty->fields.size()) << e->pos;
      for (int i = 0; i < (int)e->fields.size(); i++) {
        CHECK_EQ(e->fields[i].name, ty->fields[i].first) << e->pos;
        auto et = e_.trexp(e->fields[i].value);
        CHECK(types::is_compatible(et.ty, ty->fields[i].second)) << e->pos;
      }
      return {ty};
    }
    Expty operator()(uptr<absyn::ArrayExprAST> &e) {
      auto entry = e_.tenv.look(e->type_id);
      CHECK(entry) << e->pos << ": Undefined symbol '" << e->type_id.name()
                   << "'";
      CHECK(types::is<types::ArrayTyRef>(entry.value()))
          << e->pos << ": '" << e->type_id.name() << "' is not an array";
      auto &ty = types::as<types::ArrayTyRef>(entry.value());
      CHECK(is_int(e_.trexp(e->size))) << e->pos;
      CHECK(types::is_compatible(e_.trexp(e->init).ty, ty->base_type))
          << e->pos;
      return {ty};
    }
    Expty operator()(uptr<absyn::SeqExprAST> &e) {
      for (int i = 0; i < (int)e->exps.size() - 1; i++) {
        e_.trexp(e->exps[i].exp);
      }
      return {e_.trexp(e->exps.back().exp)};
    }
    Expty operator()(uptr<absyn::AssignExprAST> &e) {
      auto dst_ty = e_.trvar(e->var).ty;
      auto src_ty = e_.trexp(e->exp).ty;
      CHECK(is_compatible(src_ty, dst_ty)) << e->pos;
      return {dst_ty};
    }
    Expty operator()(uptr<absyn::IfExprAST> &e) {
      CHECK(is_int(e_.trexp(e->cond))) << e->pos;
      auto ty1 = e_.trexp(e->then);
      // We're ignoring the case when the result is a record type and one of
      // the branches is nil
      if (!e->else_) {
        CHECK(ty1.ty == types::UnitTy()) << e->pos;
      } else {
        auto ty2 = e_.trexp(e->else_.value());
        CHECK(ty1.ty == ty2.ty) << e->pos;
      }
      return {ty1};
    }
    Expty operator()(uptr<absyn::WhileExprAST> &e) {
      CHECK(is_int(e_.trexp(e->cond))) << e->pos;
      // We could skip this check to allow value-producing expressions in the
      // loop body, which we could simply ignore
      LoopManager::Get().EnterLoop(e.get());
      CHECK(e_.trexp(e->body).ty == types::UnitTy()) << e->pos;
      LoopManager::Get().ExitLoop();
      return {types::UnitTy()};
    }
    Expty operator()(uptr<absyn::ForExprAST> &e) {
      CHECK(is_int(e_.trexp(e->lo))) << e->pos;
      CHECK(is_int(e_.trexp(e->hi))) << e->pos;
      symbol::Scope scope(e_.tenv);
      e_.tenv.enter({e->var, types::IntTy()});
      // We could skip this check to allow value-producing expressions in the
      // loop body, which we could simply ignore
      LoopManager::Get().EnterLoop(e.get());
      // XXX: Check that e->var isn't assigned to in the body
      CHECK(e_.trexp(e->body).ty == types::UnitTy()) << e->pos;
      LoopManager::Get().ExitLoop();
      return {types::UnitTy()};
    }
    Expty operator()(uptr<absyn::BreakExprAST> &e) {
      CHECK(LoopManager::Get().IsLoop()) << e->pos;
      return {types::UnitTy()};
    }
    Expty operator()(uptr<absyn::LetExprAST> &e) {
      symbol::Scope vscope(e_.venv);
      symbol::Scope tscope(e_.tenv);
      for (auto &dec : e->decs) {
        trans_dec(e_.venv, e_.tenv, dec);
      }
      return trans_exp(e_.venv, e_.tenv, e->body);
    }
    Expty operator()(uptr<absyn::UnitExprAST> &e) { return {types::UnitTy()}; }
  };
  class VarVisitor {
    TransExp &e_;

  public:
    VarVisitor(TransExp &enclosing) : e_(enclosing) {}
    Expty operator()(uptr<absyn::SimpleVarAST> &v) {
      auto entry = e_.venv.look(v->id);
      CHECK(entry) << v->pos << ": Undefined symbol '" << v->id.name() << "'";
      CHECK(env::is<env::VarEntry>(entry.value()))
          << v->pos << ": '" << v->id.name() << "' is not a variable";
      auto &ventry = env::as<env::VarEntry>(entry.value());
      return {types::actual_ty(ventry.ty)};
    }
    Expty operator()(uptr<absyn::FieldVarAST> &v) {
      Expty et = e_.trvar(v->var);
      CHECK(is_record(et)) << v->pos;
      auto &r = types::as<types::RecordTyRef>(et.ty);
      auto it = std::find_if(r->fields.begin(), r->fields.end(),
                             [&v](const types::NamedType &field) {
                               return field.first == v->field;
                             });
      CHECK(it != r->fields.end()) << v->pos;
      return {types::actual_ty(it->second)};
    }
    Expty operator()(uptr<absyn::IndexVarAST> &v) {
      Expty et = e_.trvar(v->var);
      CHECK(is_array(et)) << v->pos;
      CHECK(is_int(e_.trexp(v->index))) << v->pos;
      auto a = types::as<types::ArrayTyRef>(et.ty);
      return {types::actual_ty(a->base_type)};
    }
  };

public:
  Expty trexp(absyn::ExprAST &e) { return std::visit(ExprVisitor(*this), e); }
  Expty trvar(absyn::VarAST &v) { return std::visit(VarVisitor(*this), v); }
  TransExp(Venv &venv, Tenv &tenv) : venv(venv), tenv(tenv) {}
};

Expty trans_exp(Venv &venv, Tenv &tenv, absyn::ExprAST &e) {
  return TransExp(venv, tenv).trexp(e);
}

class DeclVisitor {
  Venv &venv;
  Tenv &tenv;

  template <typename C, typename F>
  void check_dup(const C &c, F &&f, const char *msg) {
    std::unordered_set<const char *> names;
    for (auto &e : c) {
      CHECK_EQ(names.count(f(e)), 0U)
          << e.pos << ": Duplicate name '" << f(e) << "' in " << msg;
      names.insert(f(e));
    }
  }

public:
  DeclVisitor(Venv &venv, Tenv &tenv) : venv(venv), tenv(tenv) {}
  void operator()(uptr<absyn::VarDeclAST> &dec) {
    Expty et = trans_exp(venv, tenv, dec->init);
    if (types::is<types::NilTy>(et.ty)) {
      CHECK(dec->type_id) << dec->pos;
    }
    if (dec->type_id) {
      auto entry = tenv.look(dec->type_id->sym);
      CHECK(entry) << dec->type_id->pos;
      auto ty = types::actual_ty(entry.value());
      CHECK(is_compatible(et.ty, ty)) << dec->type_id->pos;
    }
    bool not_redec = venv.enter({dec->name, env::VarEntry{et.ty}});
    CHECK(not_redec) << dec->pos << ": Redeclaration of symbol '"
                     << dec->name.name() << "' in same scope";
  }
  void operator()(uptr<absyn::TypeDeclAST> &decs) {
    check_dup(
        decs->types, [](auto &e) { return e.name.name(); },
        "a sequence of mutually recursive types");
    for (auto &dec : decs->types) {
      auto &[name, type, pos] = dec;
      auto ty = types::make_name(name);
      bool not_redec = tenv.enter({name, ty});
      CHECK(not_redec) << dec.pos << ": Redeclaration of symbol '"
                       << dec.name.name() << "' in same scope";
    }
    for (auto &dec : decs->types) {
      auto &[name, type, pos] = dec;
      auto ty = types::as<types::NameTyRef>(tenv.look(name).value());
      ty->ty.emplace(trans_ty(tenv, type));
    }
  }
  void operator()(uptr<absyn::FuncDeclAST> &decs) {
    check_dup(
        decs->decls, [](auto &e) { return e.name.name(); },
        "a sequence of mutually recursive functions");
    for (auto &dec : decs->decls) {
      types::Ty result_ty = types::UnitTy{};
      if (dec.result) {
        auto tentry = tenv.look(dec.result->sym);
        CHECK(tentry) << dec.result->pos << ": Undefined type '"
                      << dec.result->sym.name() << "'";
        result_ty = types::actual_ty(tentry.value());
      }
      std::vector<types::Ty> formals;
      for (auto &p : dec.params) {
        auto tentry = tenv.look(p.type_id);
        CHECK(tentry) << p.pos << ": Undefined type '" << p.name.name() << "'";
        auto ty = types::actual_ty(tentry.value());
        formals.push_back(ty);
      }
      bool not_redec =
          venv.enter({dec.name, env::FunEntry{formals, result_ty}});
      CHECK(not_redec) << dec.pos << ": Redeclaration of symbol '"
                       << dec.name.name() << "' in same scope";
    }
    for (auto &dec : decs->decls) {
      check_dup(
          dec.params, [](auto &e) { return e.name.name(); },
          "function parameter list");
      symbol::Scope scope(venv);
      auto fty = env::as<env::FunEntry>(venv.look(dec.name).value());
      for (int i = 0; i < (int)dec.params.size(); i++) {
        // No need to check for duplicate here, since we just created a scope
        // and we know all the parameter names are unique, so they won't clash
        venv.enter({dec.params[i].name, env::VarEntry{fty.formals[i]}});
      }
      LoopManager::Get().EnterFun();
      Expty et = trans_exp(venv, tenv, dec.body);
      LoopManager::Get().ExitFun();
      CHECK(types::is_compatible(et.ty, fty.result))
          << dec.pos << ": Function body incompatible with declared "
          << "return type";
    }
  }
};

void trans_dec(Venv &venv, Tenv &tenv, absyn::DeclAST &decl) {
  std::visit(DeclVisitor(venv, tenv), decl);
}

class TypeVisitor {
  Tenv &tenv;

public:
  TypeVisitor(Tenv &tenv) : tenv(tenv) {}
  types::Ty operator()(uptr<absyn::NameTy> &ty) {
    auto tentry = tenv.look(ty->type_id);
    CHECK(tentry) << ty->pos;
    return tentry.value();
  }
  types::Ty operator()(uptr<absyn::RecordTy> &ty) {
    std::vector<types::NamedType> out;
    for (auto &field : ty->fields) {
      bool is_unique = std::find_if(out.begin(), out.end(), [&field](auto &n) {
                         return n.first == field.name;
                       }) == out.end();
      CHECK(is_unique) << field.pos;
      auto tentry = tenv.look(field.type_id);
      CHECK(tentry) << field.pos;
      out.emplace_back(field.type_id, tentry.value());
    }
    return types::make_record(std::move(out));
  }
  types::Ty operator()(uptr<absyn::ArrayTy> &ty) {
    auto tentry = tenv.look(ty->type_id);
    CHECK(tentry) << ty->pos;
    return types::make_array(tentry.value());
  }
};

types::Ty trans_ty(Tenv &tenv, absyn::Ty &ty) {
  return std::visit(TypeVisitor(tenv), ty);
}

} // namespace semant
