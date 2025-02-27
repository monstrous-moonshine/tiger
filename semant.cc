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

namespace detail {

void trans_dec(Venv &, Tenv &, absyn::DeclAST &);
types::Ty trans_ty(Tenv &, absyn::Ty &);

bool is_int(const Expty &et) { return types::is<types::IntTy>(et.ty); }
bool is_str(const Expty &et) { return types::is<types::StringTy>(et.ty); }
bool is_record(const Expty &et) { return types::is<types::RecordTyRef>(et.ty); }
bool is_array(const Expty &et) { return types::is<types::ArrayTyRef>(et.ty); }
bool is_nil(const Expty &et) { return types::is<types::NilTy>(et.ty); }
bool is_unit(const Expty &et) { return types::is<types::UnitTy>(et.ty); }

template <typename C, typename F>
void check_dup(const C &c, F &&f, const char *msg) {
  std::unordered_set<const char *> names;
  for (auto &e : c) {
    CHECK_EQ(names.count(f(e)), 0U)
        << e.pos << ": Duplicate name '" << f(e) << "' in " << msg;
    names.insert(f(e));
  }
}

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
        if (is_int(lhs) || is_str(lhs) || is_array(lhs)) {
          CHECK(lhs.ty == rhs.ty) << e->pos;
        } else if (is_record(lhs)) {
          CHECK(is_nil(rhs) || lhs.ty == rhs.ty) << e->pos;
        } else if (is_nil(lhs)) {
          CHECK(is_record(rhs)) << e->pos;
        } else {
          LOG_FATAL << e->pos << ": Wrong types to op";
        }
        break;
      case absyn::Op::kLt:
      case absyn::Op::kGt:
      case absyn::Op::kLe:
      case absyn::Op::kGe:
        CHECK(is_int(lhs) || is_str(lhs)) << e->pos;
        CHECK(lhs.ty == rhs.ty) << e->pos;
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
      auto aty = types::actual_ty(entry.value());
      CHECK(types::is<types::RecordTyRef>(aty))
          << e->pos << ": '" << e->type_id.name() << "' is not a record";
      auto &ty = types::as<types::RecordTyRef>(aty);
      CHECK_EQ(e->fields.size(), ty->fields.size()) << e->pos;
      for (int i = 0; i < (int)e->fields.size(); i++) {
        auto &[name, ty_] = ty->fields[i];
        CHECK_EQ(e->fields[i].name, name) << e->fields[i].pos;
        auto et = e_.trexp(e->fields[i].value);
        CHECK(types::is_compatible(et.ty, ty_)) << e->fields[i].pos;
      }
      return {ty};
    }
    Expty operator()(uptr<absyn::ArrayExprAST> &e) {
      auto entry = e_.tenv.look(e->type_id);
      CHECK(entry) << e->pos << ": Undefined symbol '" << e->type_id.name()
                   << "'";
      auto aty = types::actual_ty(entry.value());
      CHECK(types::is<types::ArrayTyRef>(aty))
          << e->pos << ": '" << e->type_id.name() << "' is not an array";
      auto &ty = types::as<types::ArrayTyRef>(aty);
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
      auto dst_et = e_.trvar(e->var);
      auto src_et = e_.trexp(e->exp);
      CHECK(!is_unit(src_et)) << e->pos;
      CHECK(is_compatible(src_et.ty, dst_et.ty)) << e->pos;
      return {types::UnitTy()};
    }
    Expty operator()(uptr<absyn::IfExprAST> &e) {
      CHECK(is_int(e_.trexp(e->cond))) << e->pos;
      auto et1 = e_.trexp(e->then);
      if (!e->else_) {
        CHECK(et1.ty == types::UnitTy()) << e->pos;
        return {types::UnitTy()};
      } else {
        auto et2 = e_.trexp(e->else_.value());
        if (is_nil(et1)) {
          CHECK(is_record(et2)) << e->pos;
          return {et2};
        } else if (is_nil(et2)) {
          CHECK(is_record(et1)) << e->pos;
          return {et1};
        } else {
          CHECK(et1.ty == et2.ty) << e->pos;
          return {et1};
        }
      }
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
      for (auto &[name, ty] : r->fields) {
        if (name == v->field)
          return {types::actual_ty(ty)};
      }
      LOG_FATAL << v->pos << ": No field '" << v->field.name() << "'";
      // dummy return
      return {types::UnitTy()};
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

class DeclVisitor {
  Venv &venv;
  Tenv &tenv;

public:
  DeclVisitor(Venv &venv, Tenv &tenv) : venv(venv), tenv(tenv) {}
  void operator()(uptr<absyn::VarDeclAST> &var) {
    Expty et = trans_exp(venv, tenv, var->init);
    auto res_ty = et.ty;
    if (types::is<types::NilTy>(et.ty)) {
      CHECK(var->type_id) << var->pos;
    }
    if (var->type_id) {
      auto entry = tenv.look(var->type_id->sym);
      CHECK(entry) << var->type_id->pos;
      CHECK(is_compatible(et.ty, entry.value())) << var->type_id->pos;
      res_ty = types::actual_ty(entry.value());
    } else {
      CHECK(!(res_ty == types::UnitTy())) << var->pos;
    }
    bool not_redec = venv.enter({var->name, env::VarEntry{res_ty}});
    CHECK(not_redec) << var->pos << ": Redeclaration of symbol '"
                     << var->name.name() << "' in same scope";
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
    check_dup(
        ty->fields, [](auto &e) { return e.name.name(); },
        "record declaration");
    std::vector<types::RTyField> out;
    for (auto &field : ty->fields) {
      auto tentry = tenv.look(field.type_id);
      CHECK(tentry) << field.pos;
      out.emplace_back(field.name, tentry.value());
    }
    return types::make_record(std::move(out));
  }
  types::Ty operator()(uptr<absyn::ArrayTy> &ty) {
    auto tentry = tenv.look(ty->type_id);
    CHECK(tentry) << ty->pos;
    return types::make_array(tentry.value());
  }
};

void trans_dec(Venv &venv, Tenv &tenv, absyn::DeclAST &decl) {
  std::visit(detail::DeclVisitor(venv, tenv), decl);
}
types::Ty trans_ty(Tenv &tenv, absyn::Ty &ty) {
  return std::visit(detail::TypeVisitor(tenv), ty);
}

} // namespace detail

Expty trans_exp(Venv &venv, Tenv &tenv, absyn::ExprAST &e) {
  return detail::TransExp(venv, tenv).trexp(e);
}

} // namespace semant
