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
  // XXX: We've eliminated NameTy before calling this.
  bool operator()(const NameTyRef &ty2) { return false; }
  bool operator()(const IntTy &ty2) { return true; }
  bool operator()(const StringTy &ty2) { return true; }
  // XXX: nil doesn't equal itself, since equality implies compatibility
  // and two nil expressions aren't compatible. This is because with two
  // nil expressions the record type can't be inferred.
  bool operator()(const NilTy &ty2) { return false; }
  bool operator()(const UnitTy &ty2) { return true; }
};

} // namespace detail

RecordTy::RecordTy(std::vector<RTyField> &&fields)
    : id(record_id++), fields(std::move(fields)) {}

ArrayTy::ArrayTy(Ty ty) : id(array_id++), base_type(ty) {}

bool operator==(const Ty &ty1, const Ty &ty2) {
  auto aty1 = actual_ty(ty1);
  auto aty2 = actual_ty(ty2);
  if (aty1.index() != aty2.index())
    return false;
  return std::visit(detail::EqualityVisitor(aty1), aty2);
}

bool is_compatible(const Ty &src, const Ty &dst) {
  if (src == dst)
    return true;
  auto aty = actual_ty(dst);
  return is<NilTy>(src) && is<RecordTyRef>(aty);
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
