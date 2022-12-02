# Makefile to compile SHA using NEON on software

CXX = g++
CXXFLAGS = -std=c++11  -march=armv8-a+crypto -Wall -O3 -DNDEBUG
LDFLAGS = 
INCLUDES = sha.h stopwatch.h

EXECUTABLES = sha
SRC = sha.cpp

.PHONY: clean

all: $(EXECUTABLES)
	@echo $(EXECUTABLES) compiled!

sha: $(SRC)
	$(CXX) -DVECTORIZED -c $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS)

clean:
	$(RM) -rf perf* *.bin *.txt *.dSYM *.out *.o *~ $(EXECUTABLES)