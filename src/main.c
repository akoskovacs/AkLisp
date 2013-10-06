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
#include <unistd.h>
#include <string.h>

#define PROMPT_MAX 10
static struct akl_state state;

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>

/* Give back a possible completion of 'text', by traversing
 through the red black tree of all global atoms. */
static char *
akl_generator(const char *text, int st)
{
    static struct akl_atom *atom;
    static size_t tlen = 0;
    /* If this is the first run, initialize the 'atom' with
      the first element of the red black tree. */
    if (!st) {
        atom = RB_MIN(ATOM_TREE, &state.ai_atom_head);
        tlen = strlen(text);
    } else {
        atom = RB_NEXT(ATOM_TREE, &state.ai_atom_head, atom);
    }

    while (atom != NULL) {
        if (strncasecmp(atom->at_name, text, tlen) == 0)
            return strdup(atom->at_name);

        atom = RB_NEXT(ATOM_TREE, &state->ai_atom_head, atom);
    }
    return NULL;
}

/* Create an array of strings, containing the possible
  completions of the given 'text', using 'akl_generator'.*/
static char **
akl_completion(char *text, int start, int end)
{
    char **matches = (char **)NULL;
    if (rl_line_buffer[start-1] == '(')
        matches = rl_completion_matches(text, akl_generator);

    return matches;
}

/* Insert the closing right brace after the fresly typed left
  brace. We should also move left the text cursor. */
static int akl_insert_rbrace(int count, int key)
{
    rl_insert_text("()");
    rl_backward_char(1, 1);
    return 0;
}

static int akl_insert_strterm(int count, int key)
{
    rl_insert_text("\"\"");
    rl_backward_char(1, 1);
    return 0;
}
static void init_readline(void)
{
    rl_readline_name = "AkLisp";
    rl_attempted_completion_function = (CPPFunction *)akl_completion;
    rl_bind_key('(', akl_insert_rbrace);
    rl_bind_key('"', akl_insert_strterm);
}
#else //HAVE_READLINE
static void init_readline(void) {}
static void add_history(char *line __unused) {}

static char *readline(char *prompt)
{
#define INPUT_BUFSIZE 256
    static char buffer[INPUT_BUFSIZE];
    int ind = 0;
    char ch;
    printf("%s", prompt);
    while ((ch = getchar()) != '\n'
        && ind < INPUT_BUFSIZE) {
        buffer[ind] = ch;
        buffer[++ind] = '\0';
    }
    return buffer;
}
#endif //HAVE_READLINE

/* Command Line parameter handling */
typedef enum {
    CMD_END,
    CMD_ASSEMBLE,
    CMD_DEFINE,
    CMD_EVAL,
    CMD_COLOR,
    CMD_COMPILE,
    CMD_INTERACTIVE,
    CMD_HELP,
    CMD_VERSION,
    CMD_UNKOWN,
    CMD_OTHER
} cmd_t;

static struct cmd_option {
    char        co_short;
    const char *co_long;
    const char *co_desc;
    bool_t      co_has_param;
    cmd_t       co_cmd;
} akl_cmd_options[] = {
    { 'a',  "assemble",    "Assemble a file",                  TRUE,  CMD_ASSEMBLE },
    { 'D',  "define",      "Define a global variable",         TRUE,  CMD_DEFINE },
    { 'e',  "eval",        "Evaluate the command line line",   TRUE,  CMD_EVAL },
    { 'n',  "no-color",    "Disable colors",                   FALSE, CMD_COLOR },
    { 'c',  "compile",     "Compile program to bytecode",      FALSE, CMD_COMPILE },
    { 'i',  "interactive", "Force interactive mode",           FALSE, CMD_INTERACTIVE },
    { 'h',  "help",        "Print this help text",             FALSE, CMD_HELP },
    { 'v',  "version",     "Print the version of the program", FALSE, CMD_VERSION },
    { 0, NULL, NULL, FALSE, CMD_END}
};

static const char **my_argv = NULL; // Could be better
static cmd_t cmd_parse(const char **args, const char **opt)
{
    const struct cmd_option *options = akl_cmd_options;
    if (my_argv == NULL)
        my_argv = ++args; /* Step over the executable name */

    while (my_argv && *my_argv) {
        /* Should be a long option, like '--help' */
        if (*(*my_argv+1) == '-' && **my_argv == '-') {
            /* Find the exact option. TODO: make it efficient */
            while (options->co_cmd != CMD_END) {
                if (strcmp(options->co_long, *my_argv+2) == 0) {
                    /* Got it! */
                    if (options->co_has_param) {
                        if (*(my_argv+1))
                            *opt = *++my_argv; /* Give back that parameter */
                        else
                            *opt = NULL;
                    }
                    my_argv++;
                    return options->co_cmd;
                }
                options++;
            }
            *opt = *++my_argv; 
            return CMD_UNKOWN;
        } else if (**my_argv == '-') {
            /* Single dash, short option */
            while (options->co_cmd != CMD_END) {
                if (*(*my_argv+1) == options->co_short) {
                    if (options->co_has_param) {
                        /* Are there any next _character_? */
                        if ((*my_argv)[2] != '\0')
                            *opt = (*my_argv)+2; /* Assign that substring */
                        /* Are there next _string_ argument? */
                        else if (*(my_argv+1))
                            *opt = *++my_argv;
                        else
                            *opt = NULL;
                    }
                    my_argv++;
                    return options->co_cmd;
                }
                options++;
            }
            *opt = *my_argv++;
            return CMD_UNKOWN;
        } else {
            /* Not even an option */
            *opt = *my_argv++;
            return CMD_OTHER;
        }
        my_argv++;
    }
    return CMD_END;
}

static void cmd_print_help(void)
{
    struct cmd_option *opt = akl_cmd_options;
    printf("usage: aklisp [switches] [programfiles] [arguments]\n");
    while (opt->co_cmd != CMD_END) {
        printf("%5c%c%5s%-10s\t%5s\n", '-', opt->co_short, "--"
            , opt->co_long, opt->co_desc);
        opt++;
    }
}

static void cmd_parse_define(const char *opt)
{
    char var[100]; /* Should be enough */
    char value[100];
    struct akl_atom *atom = NULL;
    struct akl_value *v = NULL;
    sscanf(opt, "%100[A-Za-z_+-*]=%100[A-Za-z0-9 ]", var, value);
    atom = akl_new_atom(&state, var);
    atom->at_value = akl_new_string_value(&state, value);
    printf("define %s as %s\n", var, value);
    akl_add_global_atom(&state, atom);
}

/* End of command line functions */
static void interactive_mode(void)
{
    char prompt[PROMPT_MAX];
    int lnum = 1;
    struct akl_value *value;
    char *line;
    struct akl_io_device *dev;
    struct akl_context *ctx;
    printf("Interactive AkLisp version %d.%d-%s\n"
        , VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
    printf("Copyleft (C) 2012 Akos Kovacs\n\n");
    state.ai_interactive = TRUE;
    init_readline();
    while (1) {
        snprintf(prompt, PROMPT_MAX, "[%d]> ", lnum);
            /*AKL_GRAY, lnum, AKL_END_COLOR_MARK);*/

        line = readline(prompt);
        if (line == NULL || strcmp(line, "exit") == 0) {
            printf("Bye!\n");
            free(line);
            return;
        }
        if (line && *line) {
            add_history(line);
            dev = akl_new_string_device(&state, "stdio", line);
            /*akl_list_append(in, inst->ai_program, il);*/
            ctx = akl_compile(&state, dev);
//            akl_dump_ir(ctx);
            akl_execute_ir(ctx);
            akl_dump_stack(ctx);
            printf(" => ");
            akl_print_value(&state, akl_stack_pop(ctx));
            akl_print_errors(&state);
            akl_clear_errors(&state);
            printf("\n");
        }
        lnum++;
    }
}

int main(int argc, const char *argv[])
{
    FILE *fp;
    cmd_t cmd;
    const char *opt;
#if 0
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", argv[1]);
            return -1;
        }
        // TODO...
    //    in = akl_new_file_interpreter(argv[1], fp);
        in.ai_interactive = FALSE;
    } else {
        interactive_mode();
        return 0;
    }
#endif

    akl_init_state(&state);
    akl_library_init(&state, AKL_LIB_ALL);

    if (argc == 1) {
        interactive_mode();
        return 0;
    }

    while ((cmd = cmd_parse(argv, &opt)) != CMD_END) {
        switch (cmd) {
            case CMD_ASSEMBLE:
            printf("Assembling %s\n", opt);
            break;

            case CMD_COLOR:
            state.ai_use_colors = FALSE;
            break;

            case CMD_EVAL:
            printf("Eval this line %s\n", opt);
            break;

            case CMD_HELP:
            cmd_print_help();
            break;

            case CMD_VERSION:
            printf("AkLisp v%d.%d-%s\n", VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
            break;

            case CMD_OTHER:
            printf("other: \'%s\'\n", opt);
            break;

            case CMD_UNKOWN:
            fprintf(stderr, "Unkown option \'%s\'\n", opt);
            break;

            case CMD_INTERACTIVE:
            interactive_mode();
            break;

            case CMD_DEFINE:
            if (opt != NULL)
                cmd_parse_define(opt);
            break;

            default:
//            interactive_mode();
            break;
        }
    }
    return 0;
}
