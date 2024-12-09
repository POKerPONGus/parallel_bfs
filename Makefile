CPPFLAGS := -g -pg
SRC := main.cpp
BIN := ${SRC:.cpp=.out}

HEADERS := $(wildcard *.hpp)

.PHONY: run
run: $(BIN)
	./$<

.PHONY: clean
clean: 
	rm $(OBJS) $(BIN)

$(BIN): %.out : %.cpp $(HEADERS)
	export CPLUS_INCLUDE_PATH=$$CPLUS_INCLUDE_PATH:$(shell pwd) && $(CXX) $(CPPFLAGS) $< -o $@


