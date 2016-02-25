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
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#define HAVE_EXECINFO_H
#define HAVE_UCONTEXT_H
#ifdef HAVE_EXECINFO_H
# ifdef HAVE_UCONTEXT_H
char* exe = 0;
/* get REG_EIP from ucontext.h */
#define __USE_GNU
#include <ucontext.h>
#include <execinfo.h>
#include "aklisp.h"

static int init_exec_name() 
{
    char *link = (char *)malloc(256);
    exe = (char *)malloc(256);
    snprintf(link, 256, "/proc/%d/exe", getpid());
    if(readlink(link, exe, 256)==-1) {
        fprintf(stderr,"ERRORRRRR\n");
        exit(1);
    }
    printf("Executable name initialised: %s\n",exe);
}

static const char* get_exec_name()
{
    if (exe == 0) {
        init_exec_name();
    }
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
           "from %#lx\n", sig, info->si_addr, 
           (long)uc->uc_mcontext.gregs[REG_RIP]);
  else
    printf("Got signal %d#92;\n", sig);

  trace_size = backtrace(trace, 16);
  /* overwrite sigaction with caller's address */
  trace[1] = (void *) uc->uc_mcontext.gregs[REG_RIP];

  messages = backtrace_symbols(trace, trace_size);
  /* skip first stack frame (points here) */
  printf("[backtrace] Execution path:\n");
  char syscom[256];
  for (i=1; i<trace_size; ++i)
  {
    printf("[backtrace] %s\n", messages[i]);
    sprintf(syscom,"addr2line %p -e %s -p\n", trace[i] , get_exec_name()); //last parameter is the name of this app
    system(syscom);
  }
  exit(0);
}
# endif
#endif

AKL_DEFINE_FUN(getpid, ctx, argc __unused)
{
    return AKL_NUMBER(ctx, (int)getpid());
}

AKL_DEFINE_FUN(sleep, ctx, argc)
{
    sleep(*akl_frame_pop_number(ctx));
    return &NIL_VALUE;
}

AKL_DEFINE_FUN(getenv, ctx, argc)
{
    char *env;
    env = getenv(akl_frame_pop_string(ctx));
    if (env) {
        return AKL_STRING(ctx, strdup(env));
    }
    return &NIL_VALUE;
}

AKL_DEFINE_FUN(setenv, ctx, argc)
{
    char *ename, *evalue;
    if (akl_get_args_strict(ctx, 2, AKL_VT_STRING, &ename, AKL_VT_STRING, &evalue)) {
        return NULL;
    }
    if (!ename || !evalue || !setenv(ename, evalue, 1))
        return &TRUE_VALUE;
    return &NIL_VALUE;
}

extern char **environ;
AKL_DEFINE_FUN(env, ctx, argc)
{
    char **var = environ;
    struct akl_state *s = ctx->cx_state;
    struct akl_list *vars = akl_new_list(s);
    struct akl_value *v;
    while (*var) {
       v = akl_new_string_value(s, AKL_STRDUP(*var)); 
       akl_list_append_value(s, vars, v);
       var++;
    }
    vars->is_quoted = TRUE;
    return akl_new_list_value(s, vars);
}

struct akl_module *akl_load_module_desc(struct akl_state *s, char *modpath)
{
   /* We are on our way, lazy load */
   struct stat buf;
   void *handle = dlopen(modpath, RTLD_LAZY);
   struct akl_module *mod_desc;
   char *error;
   if (!handle) {
       return NULL;
   }

   dlerror(); /* Clear last errors (if any) */
   /* Get back the module descripor, called '__module_desc' */
   mod_desc = (struct akl_module *)dlsym(handle, "__module_desc");
   if (mod_desc == NULL || ((error = dlerror()) != NULL)) {
       AKL_FREE(s, modpath);
       return NULL;
   }
   stat(modpath, &buf);
   mod_desc->am_handle = handle;
   mod_desc->am_path = modpath;
   mod_desc->am_size = buf.st_size;
   return mod_desc;
}

char *akl_get_module_path(struct akl_state *s, const char *modname)
{
   char *mod_path = (char *)akl_alloc(s
       , sizeof(AKL_MODULE_SEARCH_PATH) + strlen(modname) + 5); /* must be enough */
   strcpy(mod_path, AKL_MODULE_SEARCH_PATH);
   strcat(mod_path, modname);
   /* Ok try to figure out where is this tiny module */
   /* Firstly, start with the standard module search path (mostly the
     '/usr/share/AkLisp/modules/') */
   if (access(mod_path, R_OK) == 0) { /* Got it! */
       return mod_path;
   } else {
       /* Try with the .alm extension */
       strcat(mod_path, ".alm");
       if (access(mod_path, R_OK) == 0) {
           return mod_path;
       } else {
           /* No way! Assume that the user gave a relative or an absolute path...*/
           modname = realpath(modname, NULL); /* Get it's absolute path */
           if (access(modname, R_OK) != 0) { /* No access, give up! */
               akl_free(s, mod_path, strlen(mod_path));
               return NULL;
           }
       }
   }
}

void akl_module_free(struct akl_state *s, struct akl_module *mod)
{
    if (mod->am_handle)
        dlclose(mod->am_handle);
    AKL_FREE(s, mod);
    //akl_free(s, mod->am_path, strlen(mod->am_path));
}

/* Unfortunately, this must be a pointer */
static struct akl_state *int_state;
void interrupt_program(int sig)
{
   if (int_state->ai_interrupted) {
        exit(1);
   } else {
        int_state->ai_interrupted = TRUE;
   }
}

void akl_init_os(struct akl_state *s)
{
   int_state = s;
#ifdef HAVE_UCONTEXT_H
# ifdef HAVE_EXECINFO_H
 /* Install our signal handler */
  struct sigaction sa;

  sa.sa_sigaction = (void *)bt_sighandler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);
  signal(SIGINT, interrupt_program);
  signal(SIGSTOP, interrupt_program);
  /* ... add any other signal here */
# endif
#endif
#if 0
  AKL_ADD_CFUN(in, getpid, "GETPID", "Get the process id");
  AKL_ADD_CFUN(in, sleep, "SLEEP", "Sleeping for a given time (in seconds)");
  AKL_ADD_CFUN(in, getenv, "GETENV", "Get the value of an environment variable");
  AKL_ADD_CFUN(in, setenv, "SETENV", "Set the value of an environment variable");
  AKL_ADD_CFUN(in, env, "ENV", "Get all environment variable as a list");
  AKL_ADD_CFUN(in, load, "LOAD", "Load an AkLisp dynamic library");
  AKL_ADD_CFUN(in, unload, "UNLOAD", "Unload an AkLisp dynamic library");
#endif
}
