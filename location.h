#ifndef LOCATION_H
#define LOCATION_H
#include <sstream>
struct Location {
  int line;
  int column;
};

inline std::ostream &operator<<(std::ostream &os, const Location &pos) {
  os << pos.line << ":" << pos.column;
  return os;
}
#endif
