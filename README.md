# AkLisp
[![Build Status](https://secure.travis-ci.org/akoskovacs/AkLisp.png)][travis]

AkLisp is a tiny, hobby Lisp dialect. It is embeddable, user-friendly and relatively fast.

## About Lisp/Scheme
[Lisp](http://en.wikipedia.org/wiki/Lisp_(programming_language)) is a family of computer programming languages, with a long history and a distincitve parenthesized *Polish notation*.

Every expression is written in the form of nested lists. In Lisp-style languages, the elements of the list is surrounded by parantheses, where the first element is the function and the other elements are the arguments of the function. The list elements are separated by whitespaces. Consider the following example in C:
```c
int a = (10 + 2) * 2;
```
Which is written like this:
```lisp
(set! a (* 2 (+ 10 2)))
```
Both are set the **a** variable to 24

## Building from source
These instructions only applies to *nix systems. *Windows builds are not supported at this time.*
### Getting the code
 * with git: `git clone git://github.com/akoskovacs/AkLisp.git`
 * or download the gzipped [package](https://github.com/akoskovacs/AkLisp/archive/master.tar.gz) and unpack it.

### Compilation
Before you can compile the program, you must satisfy the build dependencies. AkLisp have three main dependencies:
 * *dl*: The dynamic linking loader library (should be installed by the default)
 * *readline*: GNU ReadLine library (Install with `sudo apt-get install libreadline5-dev libreadline5` on Debian/Ubuntu)
 * *GNU toolset*: GCC, and make (Install with `sudo apt-get install build-essential` on Debian/Ubuntu)

There are two ways to compile AkLisp from source.
#### Without CMake
This way is preferred, when you only want to use the aklisp binary (no library will be builded).
`make`
**This command will build the aklisp binary and the modules in the modules/ directory.**

#### With CMake
For this, you must install the cmake package.
```
$ sudo apt-get install cmake     # on Debian/Ubuntu
$ cmake .
$ make
```
**This will also build the aklisp binary libaklisp_static.a (static software library) libaklisp_shared.so (shared library).**

## Intresting things to try
### Interactive shell
```lisp
$ ./aklisp
Interactive AkLisp version 0.1-alpha
Copyleft (C) 2012 Akos Kovacs

[1]> (time)
(TIME)
 => (22 3 55)
[2]> 
```

### Loadable modules
```lisp
$ ./aklisp
Interactive AkLisp version 0.1-alpha
Copyleft (C) 2012 Akos Kovacs

[1]> (load "modules/dir.alm")
(LOAD "modules/dir.alm")
 => T
[2]> (glob "*.txt")
(GLOB "*.txt")
 => '("CMakeCache.txt" "CMakeLists.txt" "install_manifest.txt")
[3]> 
```

## License
AkLisp is licensed under the permissive MIT license.
```
Copyright (c) 2012 Ákos Kovács - AkLisp Lisp dialect

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```