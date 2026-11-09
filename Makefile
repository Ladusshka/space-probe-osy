CXX=g++
LD=g++
AR=ar
STRIP=strip
CXXFLAGS=-std=c++20 -Wall -pedantic -O2
SHELL:=/bin/bash
MACHINE=$(shell uname -m)-$(shell echo $$OSTYPE)

.PHONY: all deps lib clean pack

all: test


deps:
	g++ -MM *.cpp > Makefile.d

test: solution.o sample_tester.o
	$(LD) $(CXXFLAGS) -o $@ $^ -L./$(MACHINE) -lprogtest_solver -lpthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

lib:
	$(CXX) $(CXXFLAGS) -c -o progtest_solver.o progtest_solver.cpp
	mkdir -p $(MACHINE)
	$(AR) cfr $(MACHINE)/libprogtest_solver.a progtest_solver.o
	$(STRIP) --strip-unneeded $(MACHINE)/libprogtest_solver.a

clean:
	rm -f *.o test *~ core sample.tgz Makefile.d

pack: clean
	rm -f sample.tgz
	tar zcf sample.tgz --exclude progtest_solver.cpp *


-include Makefile.d
