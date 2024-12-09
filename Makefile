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
	$(CXX) $(CPPFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

