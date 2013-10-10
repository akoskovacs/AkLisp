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

AKL_DEFINE_FUN(hello, ctx, argc)
{
    printf("Hello, world from 'hello' module!\n");
    /* Every function must return something, 
      the type of 'akl_value' */
    return AKL_NIL;
}

AKL_DEFINE_FUN(string_times, ctx, argc)
{
    double t
    int i;
    const char *str;
    if (akl_get_args_strict(ctx, 2, AKL_NUMBER, &t, AKL_STRING, &str))
        /* Something didn't work, must return... */
        return AKL_NIL;

    i = (unsigned int)t;
    while (i--) {
        printf("%s", str);
    }

    return AKL_NIL;
}

AKL_DECLARE_FUNS(akl_funs) {
    AKL_FUN(hello, "hello", "A simple hello function"),
    AKL_FUN(string_times, "string-times", "Print a string n times"),
    AKL_END_FUNS()
}

/* One for initializing and one for destroying resources. */
/* Each of them should return 0 (AKL_LOAD_OK) when everything is good */
static int hello_load(struct akl_context *ctx)
{
   printf("The hello module loaded");
   /* initalize_some_resource() */
   return AKL_LOAD_OK;
}

static int hello_unload(struct akl_context *ctx)
{
   printf("The hello module unloaded");
   /* deinitalize_some_resource() */
   return AKL_LOAD_OK;
}

struct akl_module mod_hello {
    .am_name = "hello";
    .am_desc = "A simple hello module";
    .am_author = "Kovacs Akos <akoskovacs@gmx.com>"
    .am_funs = akl_funs;
    .am_load = hello_load;
    .am_unload = hello_unload;
    /* These can be NULL, when no (de)initialization needed */
    /*.am_depends_on = { "foo", "bar", NULL }; */
    .am_depends_on = NULL; /* A NULL-terminated array of other needed modules */
};

AKL_MODULE(mod_hello);
/* No need for further explaination, the code explains itself... */
