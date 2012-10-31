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
static int hello_load(struct akl_instance *in)
{
   AKL_ADD_CFUN(in, hello, "HELLO", "Hey hello!"); 
   return AKL_LOAD_OK;
}

static int hello_unload(struct akl_instance *in)
{
   AKL_REMOVE_CFUN(in, hello);
   return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(hello_load, hello_unload, "hello"
    , "A simple demo module", "Akos Kovacs");

/* No need for further explaination, the code explains itself... */
