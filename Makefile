# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11

all: csim

csim: cache_sim_main.cpp cache_sim_funcs.cpp
	$(CXX) $(CXXFLAGS) -o csim cache_sim_main.cpp cache_sim_funcs.cpp

clean:
	rm -f csim *.o