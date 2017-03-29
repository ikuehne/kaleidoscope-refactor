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

As of the time of writing, the subset of Kaleidoscope implemented consists of
extern declarations, function definitions, and arithmetic expressions.  The
following is an example of code this version can compile:

```
def myaverage(a b) (a + b) / 2
```

Compiler usage
--------------

The default compiler target is `kalc` (for KALeidoscope Compiler).  As an
example of usage (and a demonstration of its ability to produce viable object
code), save the above `myaverage` function in `kaltest.kal`.  Since the subset
of Kaleidoscope so far implemented does not allow for any interesting I/O, use
this C program to demonstrate that the compiler correctly compiles that code:

```c
#include <stdio.h>

double myaverage(double, double);

int main(void) {
    double x = 2, y = 5;

    printf("The average of %g and %g is %g.\n", x, y, myaverage(x, y));

    return 0;
}
```

Save it as ctest.c.  Then run:

```
$ ./kalc kaltest.kal --out kaltest.o
$ clang -c ctest.c
$ clang ctest.o kaltest.o -o kaltest
$ ./kaltest
The average of 2 and 5 is 3.5.
```
