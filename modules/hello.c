/* This is a demonstration of the AkLisp module capabilities. */
/* It can be loaded up by (LOAD "modules/hello.alm") */
/* You must always include this header (located under ../src/) */
#include <aklisp.h>
#include <stdio.h>

/* Modules can define simple functions with AKL_CFUN_DEFINE()
   It has the following arguments:
        o Function name (as in C)
        o A variable for the environment,
            with the type 'akl_instance' (mostly 'in')
        o A variable for the list of it's arguments, with the type
            of 'akl_list' (mostly 'args')
*/ 

AKL_CFUN_DEFINE(hello, in, args)
{
    printf("Hello, world from 'hello' module!\n");
    /* Every function must return something, 
      the type of 'akl_value' */
    return &NIL_VALUE;
}


/* Every module must have two functions with this signature */
/* Each of them should return 0 when everything is good */
static int hello_load(struct akl_instance *in)
{
   /* You can add functions to the environment with AKL_ADD_CFUN()
      It has the following arguments:
        o Instance of the interpreter
        o Name of the function (as in AKL_CFUN_DEFINE)
        o Name of the function in the interpreter
        o Description of the function */
   AKL_ADD_CFUN(in, hello, "HELLO", "Hey hello!"); 
   return AKL_LOAD_OK;
}

/* One for initializing and one for destroying resources. */
static int hello_unload(struct akl_instance *in)
{
   /* Every resource must free()'d up with AKL_REMOVE_CFUN */
   AKL_REMOVE_CFUN(in, hello);
   return AKL_LOAD_OK;
}

/* Every module must have a AKL_MODULE_DEFINE() in it.
   The first three parameters must be given, all other can be NULL
   It has the following arguments:
        o Loader code (code pointer)
        o Unloader code
        o Module name
        o Module description
        o Author
*/
AKL_MODULE_DEFINE(hello_load, hello_unload, "hello"
    , "A simple demo module", "Akos Kovacs");

