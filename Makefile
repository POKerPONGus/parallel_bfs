CPPFLAGS := -pg
SRC := main.cpp
BIN := ${SRC:.cpp=.out}
IMG_TYPE := png
GRAPH_NAME := graph

HEADERS := $(wildcard *.hpp)

.PHONY: all
all: $(GRAPH_NAME).$(IMG_TYPE)

.PHONY: clean
clean: 
	rm $(OBJS) $(BIN)

$(GRAPH_NAME).$(IMG_TYPE): %.$(IMG_TYPE) : %.dot
	dot -T$(IMG_TYPE) $< > $@

$(GRAPH_NAME).dot: $(BIN)
	./$<

$(BIN): %.out : %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

