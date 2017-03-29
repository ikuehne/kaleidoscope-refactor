CC=clang++
CXX=clang++

BOOST_OPT=/usr/local/Cellar/boost/1.62.0/lib/libboost_program_options.a
CPPFLAGS=-g $(shell llvm-config --cxxflags) -Wall -Wpedantic -std=c++14
LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs all) $(BOOST_OPT)
COMPILER_OBJS=CodeGeneratorImpl.o CodeGenerator.o Lexer.o Parser.o

all: kalc

kalc: $(COMPILER_OBJS) kalc.o

clean:
	$(RM) *.o kalc
