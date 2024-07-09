#ifndef LOCATION_H
#define LOCATION_H
struct Location {
  struct Position {
    int line;
    int column;
  };
  Position begin, end;
};
#endif
