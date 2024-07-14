CXXFLAGS := -Wall -Og -g -MMD
OUTPUT_DIR := build
SRCS := main.cc symbol.cc semant.cc types.cc
HDRS := absyn.h absyn_common.h env.h location.h logging.h print.h semant.h symbol.h token.h types.h
GENS := lex.yy.cc tiger.tab.cc
GENH := tiger.tab.hh
OBJS := $(SRCS:%.cc=$(OUTPUT_DIR)/%.o) $(GENS:%.cc=$(OUTPUT_DIR)/%.o)
DEPS := $(SRCS:%.cc=$(OUTPUT_DIR)/%.d) $(GENS:%.cc=$(OUTPUT_DIR)/%.d)

tiger: $(OBJS)
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
	clang-format -i $(SRCS) $(HDRS)

clean:
	$(RM) $(OUTPUT_DIR)/* $(GENS) $(GENH) tiger

.PHONY: clean format

-include $(DEPS)
