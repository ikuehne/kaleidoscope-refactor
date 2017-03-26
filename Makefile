CC=gcc
CXX=g++
LLVM_CMD = llvm-config --cxxflags --ldflags --system-libs --libs core
CPPFLAGS=-g $(shell $(LLVM_CMD)) -std=c++14 
RELEASE_FLAGS=-O2
DEBUG_FLAGS=-g

COMPILER_SRCS = AST.cpp Lexer.cpp Parser.cpp CodeGenerator.cpp

PARSER_DEMO_OBJS=$(subst .cpp,.o,$(parser_demo_srcs))
parser_demo_srcs = $(COMPILER_SRCS) parser_demo.cpp
codegen_demo_srcs = $(COMPILER_SRCS) codegen_demo.cpp

all: parser_demo

parser_demo: $(PARSER_DEMO_OBJS)
	$(CXX) $(CPP_FLAGS) $(DEBUG_FLAGS) -o parser_demo $(PARSER_DEMO_OBJS) 

parser_demo_depend: .parser_demo_depend

.parser_demo_depend: $(parser_demo_srcs)
	rm -rf ./.parser_demo_depend
	$(CXX) $(DEBUG_FLAGS) $(CPPFLAGS) -MM $^>>./.parser_demo_depend;

clean:
	$(RM) $(OBJS)

include .parser_demo_depend
