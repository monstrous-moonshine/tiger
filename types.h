#ifndef TYPES_H
#define TYPES_H
#include "box.h"
#include "logging.h"
#include "symbol.h"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace types {

template <typename T> using sptr = std::shared_ptr<T>;

struct IntTy {};
struct StringTy {};
struct RecordTy;
struct ArrayTy;
struct NilTy {};
struct UnitTy {};
struct NameTy;

using RecordTyRef = sptr<RecordTy>;
using ArrayTyRef = sptr<ArrayTy>;
using NameTyRef = sptr<NameTy>;
using Ty = std::variant<IntTy, StringTy, RecordTyRef, ArrayTyRef, NilTy, UnitTy,
                        NameTyRef>;
using RTyField = std::pair<symbol::Symbol, Ty>;

struct RecordTy {
  int id;

  std::vector<RTyField> fields;
  RecordTy(std::vector<RTyField> &&);
};

struct ArrayTy {
  int id;

  Ty base_type;
  ArrayTy(Ty);
};

struct NameTy {
  symbol::Symbol type_id;
  std::optional<Ty> ty;
  NameTy(symbol::Symbol t) : type_id(t) {}
  NameTy(symbol::Symbol t, Ty ty) : type_id(t), ty(ty) {}
};

inline RecordTyRef make_record(std::vector<RTyField> &&fields) {
  return std::make_shared<RecordTy>(std::move(fields));
}
inline ArrayTyRef make_array(Ty ty) { return std::make_shared<ArrayTy>(ty); }
inline NameTyRef make_name(symbol::Symbol s) {
  return std::make_shared<NameTy>(s);
}
inline NameTyRef make_name(symbol::Symbol s, Ty ty) {
  return std::make_shared<NameTy>(s, ty);
}

template <typename T, typename U> bool is(const U &v) {
  return std::holds_alternative<T>(v);
}
template <typename T, typename U> const T &as(const U &v) {
  return std::get<T>(v);
}

bool operator==(const Ty &ty1, const Ty &ty2);
bool is_compatible(const Ty &ty1, const Ty &ty2);
Ty actual_ty(const Ty &ty);

} // namespace types
#endif
