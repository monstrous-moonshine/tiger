#ifndef SYMBOL_H
#define SYMBOL_H
#include <cstdint>
#include <forward_list>
#include <optional>
#include <unordered_map>

namespace symbol {

class Symbol {
  const char *val;

public:
  explicit Symbol(const char *);
  bool operator==(const Symbol &other) const { return val == other.val; }
  explicit operator bool() const { return val != nullptr; }

  // get the underlying string; invalid after calling FreeAll
  const char *name() const { return val; }

  // release all strings from the registry
  static void FreeAll();
};

class Hash {
public:
  uint32_t operator()(const Symbol &) const;
};

class Pred {
public:
  bool operator()(const Symbol &lhs, const Symbol &rhs) const;
};

template <typename T> class Table {
public:
  using MapType = std::unordered_map<Symbol, T, Hash, Pred>;
  using value_type = typename MapType::value_type;
  Table() { begin_scope(); }
  bool enter(const value_type &v) { return table_.front().insert(v).second; }
  bool enter(value_type &&v) {
    return table_.front().insert(std::move(v)).second;
  }
  std::optional<T> look(Symbol s) {
    for (auto env = table_.begin(); env != table_.end(); env++) {
      auto it = env->find(s);
      if (it != env->end())
        return it->second;
    }
    return std::optional<T>{};
  }

private:
  std::forward_list<MapType> table_;

  void begin_scope() { table_.push_front(MapType{}); }
  void end_scope() { table_.pop_front(); }
  template <typename> friend class Scope;
};

template <typename T> class Scope {
  Table<T> &ref_;

public:
  Scope(Table<T> &ref) : ref_(ref) { ref_.begin_scope(); }
  ~Scope() { ref_.end_scope(); }
};

} // namespace symbol
#endif
