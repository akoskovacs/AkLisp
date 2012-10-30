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
#include "aklisp.h"
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#if HAVE_EXECINFO_H && HAVE_UCONTEXT_H
char* exe = 0;
/* get REG_EIP from ucontext.h */
#define __USE_GNU
#include <execinfo.h>
#include <ucontext.h>

static int init_exec_name() 
{
    char *link = (char *)malloc(256);
    char *exe = (char *)malloc(256);
    snprintf(link, 256, "/proc/%d/exe", getpid());
    if(readlink(link, exe, 256)==-1) {
        fprintf(stderr,"ERRORRRRR\n");
        exit(1);
    }
    printf("Executable name initialised: %s\n",exe);
}

static const char* get_exec_name()
{
    if (exe == 0)
        init_exec_name();
    return exe;
}

void bt_sighandler(int sig, siginfo_t *info,
                   void *secret) {

  void *trace[16];
  char **messages = (char **)NULL;
  int i, trace_size = 0;
  ucontext_t *uc = (ucontext_t *)secret;

  /* Do something useful with siginfo_t */
  if (sig == SIGSEGV)
    printf("Got signal %d, faulty address is %p, "
           "from %p\n", sig, info->si_addr, 
           uc->uc_mcontext.gregs[REG_EIP]);
  else
    printf("Got signal %d#92;\n", sig);

  trace_size = backtrace(trace, 16);
  /* overwrite sigaction with caller's address */
  trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

  messages = backtrace_symbols(trace, trace_size);
  /* skip first stack frame (points here) */
  printf("[bt] Execution path:#92;\n");
  for (i=1; i<trace_size; ++i)
  {
    printf("[bt] %s#92;\n", messages[i]);
    char syscom[256];
    sprintf(syscom,"addr2line %p -e %s", trace[i] , get_exec_name() ); //last parameter is the name of this app
    system(syscom);

  }
  exit(0);
}
#endif 

AKL_CFUN_DEFINE(getpid, in, args __unused)
{
    return akl_new_number_value(in, (int)getpid());
}

AKL_CFUN_DEFINE(sleep, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    sleep(AKL_GET_NUMBER_VALUE(a1));
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(load, in, args)
{
   void *handle;
   char *modname, *error;
   struct akl_value *a1 = AKL_FIRST_VALUE(args); 
   struct akl_module *mod_desc;
   int i, errcode = 0;

   if (!AKL_CHECK_TYPE(a1, TYPE_STRING)) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: First argument must be a string");
       return &NIL_VALUE;
   }
   modname = realpath(AKL_GET_STRING_VALUE(a1), NULL);

   for (i = 0; i < in->ai_module_count; i++) {
       mod_desc = in->ai_modules[i];
       if (mod_desc && (strcmp(modname, mod_desc->am_path) == 0)) {
           akl_add_error(in, AKL_ERROR, a1->va_lex_info
               , "ERROR: load: Module '%s' is already loaded", mod_desc->am_name);
           return &NIL_VALUE;
       }
   }

   handle = dlopen(modname, RTLD_LAZY);

   if (!handle) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: Cannot load '%s': %s", modname, dlerror());
       return &NIL_VALUE;
   }

   dlerror(); /* Clear last errors (if any) */
   mod_desc = (struct akl_module *)dlsym(handle, "__module_desc");
   if (mod_desc == NULL || ((error = dlerror()) != NULL)) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: No way to get back the module descriptor: %d", error);
       return &NIL_VALUE;
   }

   mod_desc->am_handle = handle;
   mod_desc->am_path = modname;
   errcode = mod_desc->am_load(in);
   if (errcode != AKL_LOAD_OK) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: Module loader gave error code: %d", errcode);
       return &NIL_VALUE;
   }

   if (in->ai_module_count >= in->ai_module_size) {
       in->ai_module_size += in->ai_module_size; 
       in->ai_modules = (struct akl_module **)realloc(in->ai_modules
           , (sizeof (struct akl_module))*in->ai_module_size);
   }

   in->ai_modules[in->ai_module_count] = mod_desc;
   in->ai_module_count++;
   return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(unload, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    char *modname;
    struct akl_module *mod = NULL;
    int i, errcode;
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        modname = AKL_GET_STRING_VALUE(a1);
    } else {
        return &NIL_VALUE;
    }

    for (i = 0; i < in->ai_module_size; i++) {
        mod = in->ai_modules[i];
        if (mod && ((strcmp(mod->am_path, modname) == 0)
                || (strcmp(mod->am_name, modname) == 0))) {
            errcode = mod->am_unload(in);
            if (errcode != AKL_LOAD_OK) {
               akl_add_error(in, AKL_ERROR, a1->va_lex_info
                   , "ERROR: load: Module unloader gave error code: %d, force unload", errcode);
               return &NIL_VALUE;
            }
            dlclose(mod->am_handle);
            in->ai_modules[i] = NULL;
            in->ai_module_count--;
            return &TRUE_VALUE;
        }

    }
    return &NIL_VALUE;
}

void akl_init_os(struct akl_instance *in)
{
#if HAVE_UCONTEXT_H && HAVE_EXECINFO_H
 /* Install our signal handler */
  struct sigaction sa;

  sa.sa_sigaction = (void *)bt_sighandler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);
  /* ... add any other signal here */
#endif

  AKL_ADD_CFUN(in, getpid, "GETPID", "Get the process id");
  AKL_ADD_CFUN(in, sleep, "SLEEP", "Sleeping for a given time (in seconds)");
  AKL_ADD_CFUN(in, load, "LOAD", "Load an AkLisp dynamic library");
  AKL_ADD_CFUN(in, unload, "UNLOAD", "Unload an AkLisp dynamic library");
}
