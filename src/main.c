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

#if HAVE_GETOPT_H
#include <getopt.h>
#endif // HAVE_GETOPT_H

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

#ifdef HAVE_GETOPT_H

int no_color_flag;
static struct option akl_options[] = {
    { "assemble"   , no_argument,       0, 'a' },
    { "compile"    , no_argument,       0, 'c' },
    { "eval"       , required_argument, 0, 'e' },
    { "interactive", no_argument,       0, 'i' },
    { "no-color"   , no_argument,       &no_color_flag,  1  },
    { "help"       , no_argument,       0, 'h' },
    { "version"    , no_argument,       0, 'v' },
    { NULL         ,           0,       0, 0 }
};

const char *akl_option_desc[] = {
    "Compile an AkLisp assembly file", "Compile an AkLisp program to bytecode"
    , "Evaulate a command-line expression", "Force interactive mode"
    , "Disable colors", "This help message"
    , "Print the version number"
};

void print_help(void)
{
    int i;
    printf("Usage: aklisp [options] [files] [args]\n");
    for (i = 0; akl_options[i+1].name; i++) {
        if (akl_options[i].val != 1) {
            printf("\t--%-10s\t-%c\t%s\n", akl_options[i].name
                   , akl_options[i].val, akl_option_desc[i]);
        } else {
            printf("\t--%-15s\t%s\n", akl_options[i].name
                   , akl_option_desc[i]);
        }
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
#endif // HAVE_GETOPT_H

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
    int c;
    int opt_index = 0;
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

#ifdef HAVE_GETOPT_H
    while((c = getopt_long(argc, argv, "ae:chiv", akl_options, &opt_index)) != -1) {
        if (no_color_flag)
            state.ai_use_colors = FALSE;

        switch (c) {
            case 'a':
            printf("Assembling\n");
            break;

            case 'e':
            printf("Eval this line \'%s\'\n", optarg);
            break;

            case 'h':
            print_help();
            break;

            case 'v':
            printf("AkLisp v%d.%d-%s\n", VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
            break;

            case 'D':
                cmd_parse_define(optarg);
            break;

            case 'i':
            default:
            interactive_mode();
            break;
        }
    }
#endif // HAVE_GETOPT_H
    return 0;
}
