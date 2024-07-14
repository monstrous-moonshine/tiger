#include "types.h"

namespace {
int record_id = 0;
int array_id = 0;
} // namespace

namespace types {
namespace detail {

class EqualityVisitor {
  const Ty &ty1;

public:
  EqualityVisitor(const Ty &ty1) : ty1(ty1) {}
  bool operator()(const RecordTyRef &ty2) {
    return as<RecordTyRef>(ty1)->id == ty2->id;
  }
  bool operator()(const ArrayTyRef &ty2) {
    return as<ArrayTyRef>(ty1)->id == ty2->id;
  }
  bool operator()(const NameTyRef &ty2) {
    CHECK(0) << ty2->type_id.name() << ": Unexpected NameTy";
    return as<NameTyRef>(ty1)->type_id == ty2->type_id;
  }
  bool operator()(const IntTy &ty2) { return true; }
  bool operator()(const StringTy &ty2) { return true; }
  bool operator()(const NilTy &ty2) { return true; }
  bool operator()(const UnitTy &ty2) { return true; }
};

} // namespace detail

RecordTy::RecordTy(std::vector<NamedType> &&fields)
    : id(record_id++), fields(std::move(fields)) {}

ArrayTy::ArrayTy(Ty ty) : id(array_id++), base_type(ty) {}

bool operator==(const Ty &ty1, const Ty &ty2) {
  if (ty1.index() != ty2.index())
    return false;
  return std::visit(detail::EqualityVisitor(ty1), ty2);
}

bool is_compatible(const Ty &ty1, const Ty &ty2) {
  if (ty1 == ty2)
    return true;
  if (is<NilTy>(ty1) && is<RecordTyRef>(ty2)) {
    return true;
  }
  return false;
}

Ty actual_ty(const Ty &ty) {
  Ty ty1 = ty;
  NameTyRef *t = nullptr;
  while ((t = std::get_if<NameTyRef>(&ty1))) {
    CHECK((*t)->ty) << (*t)->type_id.name() << ": Incomplete type";
    ty1 = (*t)->ty.value();
  }
  return ty1;
}

} // namespace types
