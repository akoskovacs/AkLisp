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

AKL_CFUN_DEFINE(getenv, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    char *env;
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        env = getenv(AKL_GET_STRING_VALUE(a1));
        if (env)
            return akl_new_string_value(in, env);
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(setenv, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_value *a2 = AKL_SECOND_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)
        && AKL_CHECK_TYPE(a2, TYPE_STRING)) {
        if (!setenv(AKL_GET_STRING_VALUE(a1)
                , AKL_GET_STRING_VALUE(a2), 1))
            return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

extern char **environ;
AKL_CFUN_DEFINE(env, in, args)
{
    struct akl_list *vars = akl_new_list(in);
    struct akl_value *v;
    char **var = environ;
    while (*var) {
       v = akl_new_string_value(in, strdup(*var)); 
       akl_list_append(in, vars, v);
       var++;
    }
    vars->is_quoted = TRUE;
    return akl_new_list_value(in, vars);
}

AKL_CFUN_DEFINE(load, in, args)
{
   void *handle;
   char *modname, *error;
   struct akl_value *a1 = AKL_FIRST_VALUE(args); 
   struct akl_module *mod_desc;
   char *mod_path;
   int i, errcode = 0;

   if (!AKL_CHECK_TYPE(a1, TYPE_STRING)) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: First argument must be a string.\n");
       return &NIL_VALUE;
   }

   modname = AKL_GET_STRING_VALUE(a1);
   mod_path = (char *)akl_malloc(in
       , sizeof(AKL_MODULE_SEARCH_PATH) + strlen(modname) + 5); /* must be enough */
   strcpy(mod_path, AKL_MODULE_SEARCH_PATH);
   strcat(mod_path, modname);
   /* Ok try to figure out where is this tiny module */
   /* Firstly, start with the standard module search path (mostly the
     '/usr/share/AkLisp/modules/') */
   if (access(mod_path, R_OK) == 0) { /* Got it! */
       modname = mod_path;
   } else {
       /* Try with the .alm extension */
       strcat(mod_path, ".alm");
       if (access(mod_path, R_OK) == 0) {
           modname = mod_path;
       } else {
           /* No way! Assume that the user gave a relative or an absolute path...*/
           modname = realpath(modname, NULL); /* Get it's absolute path */
           if (access(modname, R_OK) != 0) { /* No access, give up! */
               akl_add_error(in, AKL_ERROR, a1->va_lex_info
                   , "ERROR: load: Module '%s' is not found.\n"
                   , AKL_GET_STRING_VALUE(a1));
               FREE_FUNCTION((void *)mod_path);
               return &NIL_VALUE;
           }
       }
   }

   /* Are there any of this module already loaded in? */
   /* NOTE: Just the paths are examined here */
   for (i = 0; i < in->ai_module_size; i++) {
       mod_desc = in->ai_modules[i];
       /* TODO: Compare the basename()'s of the paths */
       if (mod_desc && (strcmp(modname, mod_desc->am_path) == 0)) {
           akl_add_error(in, AKL_ERROR, a1->va_lex_info
               , "ERROR: load: Module '%s' is already loaded.\n", mod_desc->am_name);
           FREE_FUNCTION((void *)mod_path);
           return &NIL_VALUE;
       }
   }

   /* We are on our way, lazy load */
   handle = dlopen(modname, RTLD_LAZY);
   if (!handle) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: Cannot load '%s': %s\n", modname, dlerror());
       FREE_FUNCTION((void *)mod_path);
       return &NIL_VALUE;
   }

   dlerror(); /* Clear last errors (if any) */
   /* Get back the module descripor, called '__module_desc' */
   mod_desc = (struct akl_module *)dlsym(handle, "__module_desc");
   if (mod_desc == NULL || ((error = dlerror()) != NULL)) {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: No way to get back the module descriptor: %d.\n", error);
       FREE_FUNCTION((void *)mod_path);
       return &NIL_VALUE;
   }

   /* Assign some value to the important fields */
   mod_desc->am_handle = handle;
   mod_desc->am_path = modname;
   if (mod_desc->am_load && mod_desc->am_unload && mod_desc->am_name) {
       /* Start the loader... */
       errcode = mod_desc->am_load(in);
       if (errcode != AKL_LOAD_OK) {
           akl_add_error(in, AKL_ERROR, a1->va_lex_info
               , "ERROR: load: Module loader gave error code: %d.\n", errcode);
           FREE_FUNCTION((void *)mod_path);
           dlclose(handle);
           return &NIL_VALUE;
       }
   } else {
       akl_add_error(in, AKL_ERROR, a1->va_lex_info
           , "ERROR: load: Module descriptor is invalid.\n");
       FREE_FUNCTION((void *)mod_path);
       dlclose(handle);
       return &NIL_VALUE;
   }

   /* No free slot for the new module, make some room */
   if (in->ai_module_count >= in->ai_module_size-1) {
       in->ai_module_size += in->ai_module_size; 
       in->ai_modules = (struct akl_module **)realloc(in->ai_modules
           , (sizeof (struct akl_module *))*in->ai_module_size);
   }
    
   in->ai_module_count++;

   /* Are there any unloaded module, with a free slot */
   for (i = 0; i < in->ai_module_size; i++) {
       if (in->ai_modules[i] == NULL) {
           in->ai_modules[i] = mod_desc;
           return &TRUE_VALUE;
       }
   }
   return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(unload, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_value *a2;
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
        /* We can only search by name */
        if (mod && ((strcmp(mod->am_name, modname) == 0)
                || (strcmp(mod->am_path, modname) == 0))) {
            if (mod->am_unload) {
                errcode = mod->am_unload(in);
                if (errcode != AKL_LOAD_OK) {
                   if (args->li_elem_count < 2) {
                       akl_add_error(in, AKL_ERROR, a1->va_lex_info
                       , "ERROR: unload: Module unloader gave error code: %d.\n", errcode);
                       akl_add_error(in, AKL_WARNING, a1->va_lex_info
                       , "Use :force symbol as the second argument to force unload.\n");
                       return &NIL_VALUE;
                   } else {
                       a2 = AKL_SECOND_VALUE(args);
                       if (AKL_CHECK_TYPE(a2, TYPE_ATOM) && AKL_IS_QUOTED(a2)) {
                           if (strcmp(a2->va_value.atom->at_name, "FORCE") != 0) {
                               akl_add_error(in, AKL_ERROR, a2->va_lex_info
                               , "ERROR: unload: Unknown option.\n");
                               return &NIL_VALUE;
                           }
                           /* Force unload */
                       } else {
                           akl_add_error(in, AKL_ERROR, a2->va_lex_info
                           , "ERROR: unload: Unknown option.\n");
                           return &NIL_VALUE;
                       }
                   }
                }
            } else {
               akl_add_error(in, AKL_ERROR, a1->va_lex_info
                   , "ERROR: unload: Cannot call '%s' module's unload code\n"
                   , mod->am_name);
               return &NIL_VALUE;
            }
            FREE_FUNCTION((void *)mod->am_path);
            dlclose(mod->am_handle);
            in->ai_modules[i] = NULL;
            in->ai_module_count--;
            return &TRUE_VALUE;
        }

    }
    akl_add_error(in, AKL_ERROR, a1->va_lex_info
       , "ERROR: unload: '%s' module not found.\n"
       , modname);
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
  AKL_ADD_CFUN(in, getenv, "GETENV", "Get the value of an environment variable");
  AKL_ADD_CFUN(in, setenv, "SETENV", "Set the value of an environment variable");
  AKL_ADD_CFUN(in, env, "ENV", "Get all environment variable as a list");
  AKL_ADD_CFUN(in, load, "LOAD", "Load an AkLisp dynamic library");
  AKL_ADD_CFUN(in, unload, "UNLOAD", "Unload an AkLisp dynamic library");
}
