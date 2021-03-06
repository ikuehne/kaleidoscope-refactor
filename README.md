Kaleidoscope Refactor
=====================

A rewrite of the [LLVM Kaleidoscope tutorial](
http://llvm.org/docs/tutorial/index.html) mostly aimed at removing global
variables; making the code prettier, more extensible, and more maintainable; and
most importantly at learning LLVM by forcing myself to carefully read the
tutorial and understand the code.

Some major changes from the source code listing in the tutorial:

 * Breaking up code into classes and files.
 * Representing AST using boost::variants, and doing code generation with a
   visitor.
 * Static instead of JIT compilation.
 * More sensible command-line interface.
 * Better use of C++11 and C++14 features.
 * Helpful error messages with coloring and source snippets.

Building
--------

Since this is just an instructive project, portability and ease of compilation
are not major goals.  The only system on which this code is known to compile and
run uses:

 * `clang` version `8.0.0`.
 * `llvm` version `3.8.1`.
 * `boost` version `1.62.0`.
 * Mac OSX Sierra (on Intel).

In theory, there is no reason the compiler should not run on any platform LLVM
supports, but of course it is untested on most such platforms.

The best advice I have for compiling this project is to install the tools listed
above and change the `BOOST_OPT` variable in the Makefile, which should point to
the binary part of the Boost.Program\_options library.  Then just try to `make`
and see what happens.  Good luck.

Language
--------

The subset of Kaleidoscope implemented consists of extern declarations, function
definitions, arithmetic expressions, and conditionals.  The following is an
example of code this version can compile:

```
def fibonacciaux(x1 x2 n)
    if (n < 0.5) then x1
                 else fibonacciaux(x2, x1 + x2, n - 1)

def fibonacci(n) fibonacciaux(0, 1, n)
```

Compiler usage
--------------

The default compiler target is `kalc` (for KALeidoscope Compiler).  As an
example of usage (and a demonstration of its ability to produce viable object
code), save the above `myaverage` function in `fibonacci.kal`.  Since the subset
of Kaleidoscope so far implemented does not allow for any interesting I/O, use
this C program to demonstrate that the compiler correctly compiles that code:

```c
#include <stdio.h>

double fibonacci(double);

int main(void) {
    double i;

    for (i = 0; i < 11; i += 1.0) {
        printf("%gth fibonacci:\t%g\n", i, fibonacci(i));
    }

    return 0;
}

```

Save it as test.c.  Then run:

```
$ ./kalc fibonacci.kal --obj fibonacci.o
$ clang -c test.c
$ clang test.o fibonacci.o -o test
$ ./test
0th fibonacci:	0
1th fibonacci:	1
2th fibonacci:	1
3th fibonacci:	2
4th fibonacci:	3
5th fibonacci:	5
6th fibonacci:	8
7th fibonacci:	13
8th fibonacci:	21
9th fibonacci:	34
10th fibonacci:	55
```

To show `kalc`'s nice error printing, add some problems to fibonacci.kal:

```
def if

def fibonacciaux(x1 x2 n)
    if (n < 0.5) then x1
                 else fibonacciaux(x2, x1 + x2, n - 1)

def fibonacci(n) fibonacciaux(0, 1, n)
```

When we attempt to compile this, `kalc` gives us nicely formatted error
messages:

![errors](assets/error_demo.png)
