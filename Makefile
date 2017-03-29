CC=clang++
CXX=clang++

BOOST_OPT=/usr/local/Cellar/boost/1.62.0/lib/libboost_program_options.a
CPPFLAGS=-g $(shell llvm-config --cxxflags) -Wall -Wpedantic -std=c++14
LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs all) $(BOOST_OPT)
COMPILER_OBJS=CodeGeneratorImpl.o CodeGenerator.o Lexer.o Parser.o

all: parser_demo codegen_demo kalc

kalc: main
	mv main kalc

main: $(COMPILER_OBJS) main.o

parser_demo: $(COMPILER_OBJS) parser_demo.o

codegen_demo: $(COMPILER_OBJS) codegen_demo.o

clean:
	$(RM) *.o parser_demo codegen_demo main kalc
