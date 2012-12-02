/************************************************************************
 *   Copyright (c) 2012 Ákos Kovács - AkLisp Lisp dialect
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ************************************************************************/

/* This is a demonstration of the AkLisp module capabilities. */
#include <aklisp.h>
#include <stdio.h>

AKL_CFUN_DEFINE(hello, in, args)
{
    printf("Hello, world from 'hello' module!\n");
    /* Every function must return something, 
      the type of 'akl_value' */
    return &NIL_VALUE;
}


/* Every module must have two functions with this signature */
/* One for initializing and one for destroying resources. */
/* Each of them should return 0 when everything is good */
static int hello_load(struct akl_state *in)
{
   AKL_ADD_CFUN(in, hello, "HELLO", "Hey hello!"); 
   return AKL_LOAD_OK;
}

static int hello_unload(struct akl_state *in)
{
   AKL_REMOVE_CFUN(in, hello);
   return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(hello_load, hello_unload, "hello"
    , "A simple demo module", "Akos Kovacs");

/* No need for further explaination, the code explains itself... */
