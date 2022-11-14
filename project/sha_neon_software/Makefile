# Makefile to compile SHA using NEON on software

CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3 -mcpu=native -DNDEBUG 
LDFLAGS = 
INCLUDES = Server/SHA/sha.h

EXECUTABLES = sha
SRC = Server/SHA/sha.cpp

.PHONY: clean

all: $(EXECUTABLES)
	@echo $(EXECUTABLES) compiled!

sha: $(SRC)
	$(CXX) -DVECTORIZED -c $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS)

clean:
	$(RM) -rf perf* *.bin *.txt *.dSYM *.out *.o *~ $(EXECUTABLES)