CC=gcc
CXX=g++
CPPFLAGS=-g -std=c++14
LLVM_FLAGS=-lllvm
RELEASE_FLAGS=-O2
DEBUG_FLAGS=-g

OBJS=$(subst .cpp,.o,$(SRCS))
SRCS=$(wildcard *.cpp)

all: parser_demo

parser_demo: $(OBJS)
	$(CXX) $(DEBUG_FLAGS) -o parser_demo $(OBJS)

depend: .depend

.depend: $(SRCS)
	rm -rf ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

include .depend
