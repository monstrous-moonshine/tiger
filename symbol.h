#ifndef SYMBOL_H
#define SYMBOL_H
class Symbol {
  const char *val;

public:
  explicit Symbol(const char *);
  bool operator==(const Symbol &other) const {
    return val == other.val;
  }
  explicit operator bool() const {
    return val != nullptr;
  }

  // get the underlying string; invalid after calling FreeAll
  const char *str() const { return val; }

  // release all strings from the registry
  static void FreeAll();
};
#endif
