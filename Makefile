CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2

all: tea

tea: tea.cpp
	$(CXX) $(CXXFLAGS) -o $@ tea.cpp

test: tea
	sh tests/run.sh ./tea

clean:
	rm -f tea

.PHONY: all test clean
