#include "symbol.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_set>

namespace {
class Hash {
  using T = const char *;

public:
  uint32_t operator()(const T &str) const {
    uint32_t hash = 2166136261u;
    for (int i = 0; str[i] != '\0'; i++) {
      hash ^= static_cast<uint8_t>(str[i]);
      hash *= 16777619;
    }
    return hash;
  }
};
class Pred {
  using T = const char *;

public:
  bool operator()(const T &lhs, const T &rhs) const {
    return strcmp(lhs, rhs) == 0;
  }
};
std::unordered_set<const char *, Hash, Pred> registry;
} // namespace

namespace symbol {

// Strings passed to this constructor are allocated by the lexer. If it's new,
// it's stashed away in the registry. Symbol itself doesn't own the string --
// the registry does. If the string (the same sequence of characters, not the
// same pointer) was seen before, it's released and the previously seen string
// is used, guaranteeing uniqueness.
Symbol::Symbol(const char *val) {
  if (val == nullptr) {
    this->val = nullptr;
    return;
  }
  auto it = registry.find(val);
  if (it == registry.end()) {
    registry.insert(val);
    this->val = val;
  } else {
    this->val = *it;
    std::free(const_cast<char *>(val));
  }
}

uint32_t Hash::operator()(const Symbol &s1) const {
  return ::Hash{}(s1.name());
}

bool Pred::operator()(const Symbol &s1, const Symbol &s2) const {
  return ::Pred{}(s1.name(), s2.name());
}

void Symbol::FreeAll() {
  for (auto *str : registry) {
    std::free(const_cast<char *>(str));
  }
  // remove the dangling pointers; all existing Symbols are invalid now except
  // for equality comparison
  registry.clear();
}

} // namespace symbol
