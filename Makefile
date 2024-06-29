CXXFLAGS := -Wall -Og -g -MMD
OUTPUT_DIR := build
OBJECTS := main.o lex.yy.o tiger.tab.o symbol.o
DEPS := $(OBJECTS:%.o=$(OUTPUT_DIR)/%.d)

tiger: $(OBJECTS:%=$(OUTPUT_DIR)/%)
	$(CXX) -o $@ $^

$(OUTPUT_DIR)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

lex.yy.cc: tiger.l
	flex -o$@ $<

tiger.tab.hh: tiger.tab.cc
	touch $@

tiger.tab.cc: tiger.yy
	bison -d $<

$(OUTPUT_DIR)/main.o $(OUTPUT_DIR)/lex.yy.o: tiger.tab.hh

format:
	clang-format -i absyn.h symbol.* token.h

clean:
	$(RM) $(OUTPUT_DIR)/* lex.yy.cc tiger.tab.{cc,hh} tiger

.PHONY: clean format

-include $(DEPS)
