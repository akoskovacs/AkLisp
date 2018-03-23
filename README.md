# AkLisp [![Build Status](https://secure.travis-ci.org/akoskovacs/AkLisp.png)](http://travis-ci.org/akoskovacs/AkLisp)
AkLisp is a tiny, hobby Lisp dialect. It is embeddable, user-friendly and relatively fast.

```
WARNING! This is an experimental (wip: Work in progress) branch of AkLisp.
Code might not work or not even build.
```

## About AkLisp
> [Lisp](http://en.wikipedia.org/wiki/Lisp_(programming_language)) is a family of computer programming languages, with a long history and a distincitve parenthesized *Polish notation*.

**AkLisp is a Lisp/Scheme style language, without backward compatibility.**

Every expression is formed as a nested lists. In Lisp-style languages, the elements are surrounded with parantheses, where the first entry is the function while the following elements are the arguments of the function. The list elements are separated by whitespaces. Consider the following example in C:
```c
int a = (10 + 2) * 2;
```
Which is expressed in Lisp as:
```lisp
(set! a (* 2 (+ 10 2)))
```
Both set the *a* variable to 24. The biggest difference is the that C uses infix, while `AkLisp` uses the prefix format. The `!` at the end of the function name is only a notion, that signifies that the function could change its arguments or the interpreter's context (like the variables). *`AkLisp` currently only supports global variables and function parameters.*

Every list is automatically evaluated if they are not quoted. To quote a list, just write a \' before the list, like:
```lisp
'(+ 1 2 3) ; Will be treated as data
```
Every list is implemented as a singly linked list in the interpreter. You can devide lists to a head and tail. The head is the first element. And the tail is a list of the subsequent elements.
```lisp
(head '(:one 2 3 :four))            ; => :one, the first element
; or
(first '(:one 2 3 :four))            ; => :one, the first element
(tail '(:one 2 3 :four))            ; => '(2 3 :four), the subsequent elements
; Therefore the second element can be easy gathered with:
(head (tail '(:one 2 3 :four)))     ; => 2, the second element
```
`:one`, `:four` are Ruby-like symbols used as lightweight identifiers for data. These are only created once, but could be converted to a string with the `string` function.

To encourage code reuse, you can define functions, just as in most high-level languages. :sunglasses:

This example shows a function, called `say-hello` which has an empty parameter list `()` and a body `(display "Hello, world!")`. 

```lisp
(defun! say-hello () (display "Hello, world!"))
(say-hello)
; prints: "Hello, world!"
```

Because AkLisp uses the postfix format for function calls and the usual operators, like `+`, `-`, ... are just functions, names could contain special characters that most languages doesn't allow as variable and function names, like the `-` here. 

Functions are even better when used with parameters:

```lisp
(defun! say-hello (name) (display "Hello, " name "!"))
(say-hello "Robert")
; prints: "Hello, Robert!"
(say-hello "John")
; prints: "Hello, John!"
```

For example, some functions which are **special-form** *(like `defun!`)* use an `!` at the end of their name to show that **they are evaluated at compilation** instead of evaulation time, unlike functions.

Functions themselves could be passed to functions as a parameter, too:
```lisp
(times 4 say-hello)
; returns: '(T T T)
; prints:
; "Hello, world
; "Hello, world
; "Hello, world
; "Hello, world
```

Special-forms are also used to implement decisions in code.
```lisp
(defun! say-hello (name)
  (if (nil? name)
    (display "I need a name, pls!")
    (display "Hello, " name "!")
  )
)
(say-hello "Robert")
; prints: "Hello, Robert!"
(say-hello "")
; prints: "I need a name, pls!")
(say-hello)
; prints: "I need a name, pls!")
```
If the name is `NIL` *(undefined, ungiven or empty string)* we write out an error, otherwise we proceed just like above. 

> `if` works similarly to other languages *(except that, there is no else keyword)*, but here it is implemented as a built-in special-form and looks like a function with three parameters.

The second parameter evaulated if the first parameter is not `NIL`, otherwise the third will be executed. 

`nil?` is a predicate function which returns `T` *(true value)* if its only argument is `NIL`.

`when` is similar to `if` but it only needs the true branch.
```lisp
(defun! print-even (n) 
  (when (= (mod n 2) 0)
    (print n)
  )
)
```
`(mod n 2)` will compute the remainder or the `n/2` operation. If `n` is even its remainder will be zero. We test this with the `=` predicate, which return `T` if both of its arguments are the same.
```lisp
(print-even 20)
; prints: "20"
(print-even 42)
; prints: "42"
(print-even 11)
; prints nothing, returns NIL
(print-even 5)
; prints nothing, returns NIL
```
To test more numbers we can generate a list with `range`, then go through that list with `map`, passing the new `print-even` function.
```lisp
(range -4 7)
; returns the list: '(-4 -3 -2 -1 0 1 2 3 4 5 6 7)
(map (range -4 7) print-even)
; prints:
; -4
; -2
; 0
; 2
; 4
; 6
```
As expected we got a list of even numbers between `-4` and `7`.

AkLisp also supports recursive functions, like the famous factorial implementation:
```lisp
(defun! fact (n)
        (if (<= n 1)
          1
          (* n (fact (-- n)))
        )
)
(fact 4)
; => 24
```
If the number `n` is smaller or equal to `1`, we return `1`, otherwise the current `n` gets multiplied to the factorial of `n-1` *(here as `(-- n)`, `--` is the decrement by one function)*.

`fold` usually also present in functional languages works too, however to this date, only the left fold is implemented. Also known as `foldl`. `fold` is a direct alias of `foldl`.

```lisp
(defun! sum (xs) (fold 0 xs +))

(sum (range 1 10))
; => 55
```
Here `fold` is used to add up all the numbers from `xs`. The parameters of these functions are the following:

1. Starting value.
2. The list which will fold from.
3. Binary operator used for folding.

At first the starting value will be passed to as the first argument of the binary operator with the head of the `xs` list.


Then the value returned by the operator will be passed as the first parameter of the operator with the next value from the list and so on:
```lisp
(... (+ (+ 0 (head xs) (head (tail xs))) (tail (tail (head xs)))) ...)
```
## Internals

Every function is compiled to a small bytecode representation inside the interpreter.

```lisp
(defun! fact (n)
        (if (<= n 1)
          1
          (* n (fact (-- n)))
        )
)
```

When `defun!` is seen by the parser, the built-in special form `defun` from `src/lib_spec.c` is immidately called and builds the internal bytecode representation for the new function, which could be later retrived with:
```lisp
(disassemble :fact) 
```
Which dumps something similar to this:
```as
.fact:
	load %0
	push 1
	call <=, 2
	jn .L1
	push 1
	jmp .L0
.L1:
	load %0
	load %0
	call --, 1
	call fact, 1
	call *, 2
.L0:
	nop
```
An incomplete list of the operations performed, is shown here:

| Operation code | Example      | Meaning                                                             |
|----------------|--------------|---------------------------------------------------------------------|
| **load**       | `load %0`    | Loads the first parameter to the stack                              |
| **push**       | `push 1`     | Push the number `1` to the stack                                    |
| **call**       | `call <=, 2` | Call function `<=` with `2` parameters pushed to the stack          |
| **jn**         | `jn .L1`     | Jump to label `.L1`, if the value at the top of the stack is `NIL`  |
| **jmp**        | `jmp .L0`    | Jump to label `.L0` without condition                               |

The bytecode is appended to an internal list by the `akl_build_*()` functions. Then the virtual machine defined in `src/aklisp.c` executes the emitted bytecode instructions:
```c
static void
akl_ir_exec_branch(struct akl_context *ctx, struct akl_list_entry *ip)
{
  /* ... */
  struct akl_ir_instruction *in;
  /* ... execute the instructions in the current branch and context ... */
  while (ip) {
      in = (struct akl_ir_instruction *)ip->le_data;
      switch (in->in_op) {
        /* ... case for each instruction ... */
      }
}
```

## Building from source
These instructions only applies to unix-style systems. **Windows builds are not supported at this time.**
### Getting the code
 * with git: `git clone git://github.com/akoskovacs/AkLisp.git`
 * or download the gzipped [package](https://github.com/akoskovacs/AkLisp/archive/master.tar.gz) and unpack it.

### Compilation
Before you can compile the program, you must satisfy the build dependencies. AkLisp have three main dependencies:
 * *dl*: The dynamic linking loader library (should be installed by default)
 * *readline*: GNU ReadLine library (Install with `sudo apt-get install libreadline5-dev libreadline5` on Debian/Ubuntu)
 * *GNU toolset*: GCC, and make (Install with `sudo apt-get install build-essential` on Debian/Ubuntu)

There are two ways to compile AkLisp from source.
#### Without CMake
This way is preferred when you only want to use the aklisp binary (no library will be builded).
```
$ make
```
*This command will build the aklisp binary and the modules in the modules/ directory.*

#### With CMake
For this, you must install the cmake package.
```
$ sudo apt-get install cmake libreadline-dev build-essential    # on Debian/Ubuntu
$ cmake .
$ make
```
*This will also build the aklisp binary libaklisp_static.a (static software library) libaklisp_shared.so (shared library).*

## Interesting things to try
### Interactive shell
When no arguments are given the executable will start an interactive shell. This is the most convenient way to try the interpreter. If the executable linked with the ReadLine library, you can also use tab-completition between the parentheses.
```lisp
$ ./aklisp
Interactive AkLisp version 0.2-prealpha
Copyleft (C) 2017 Akos Kovacs

[1]> (display "Hello, " (read-string) "!")
Ákos
Hello, Ákos!
 => T
[2]> 
```

### Loadable modules
> Currently this is broken and turned off. :(
```lisp
$ ./aklisp
Interactive AkLisp version 0.2-prealpha
Copyleft (C) 2017 Akos Kovacs

[1]> (load "modules/dir.alm")
 => T
[2]> (glob "*.txt")
 => '("CMakeCache.txt" "CMakeLists.txt" "install_manifest.txt")
```

## Tasks
  - [x] Basic functions and operations
  - [x] Basic code completitions in REPL *(still buggy)*
  - [x] No way to define functions [bug #2](https://github.com/akoskovacs/AkLisp/issues/2)
  - [x] Bytecode emitter [bug #5](https://github.com/akoskovacs/AkLisp/issues/5)
  - [x] Bytecode assembly printer
  - [x] Virtual machine
  - [ ] Saveable and executable binary bytecode file representation
  - [ ] Working GC *(garbage collector)* *[partly done]*
  - [ ] Testing framework *[partly done]*
  - [ ] Bytecode assembly parser *[implementation started]*
  - [ ] Make C modules work again
  - [ ] Bytecode optimization
  - [ ] Windows?
  - [ ] Better documentation
  - [ ] `nop` elimination
  - [ ] Bugs, bugs, bugs...

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
