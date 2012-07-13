#include "aklisp.h"
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_EXECINFO_H && HAVE_UCONTEXT_H
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

static struct akl_value *getpid_function(struct akl_instance *in
                                  , struct akl_list *args __unused)
{
    return akl_new_number_value(in, (int)getpid());
}

void akl_init_os(struct akl_instance *in)
{
#ifdef HAVE_UCONTEXT_H && HAVE_EXECINFO_H
 /* Install our signal handler */
  struct sigaction sa;

  sa.sa_sigaction = (void *)bt_sighandler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);
  /* ... add any other signal here */
#endif

  akl_add_global_cfun(in, "GETPID", getpid_function, "Get the process id");
}
