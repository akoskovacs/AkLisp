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
#include "../config.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif // HAVE_GETOPT_H

#define ALWAYS_DUMP_STACK 1
#define ALWAYS_DUMP_INSTR 1

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
    static struct akl_symbol *sym;
    static size_t tlen = 0;
    /* If this is the first run, initialize the 'atom' with
      the first element of the red black tree. */
    if (!st) {
        sym = RB_MIN(SYM_TREE, &state.ai_symbols);
        tlen = strlen(text);
    } else {
        sym = RB_NEXT(SYM_TREE, &state.ai_symbols, sym);
    }

    while (sym != NULL) {
        if (strncasecmp(sym->sb_name, text, tlen) == 0)
            return strdup(sym->sb_name);

        sym = RB_NEXT(SYM_TREE, &state.ai_symbols, sym);
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

static int akl_char_delete(int count, int key)
{
    int i;
    int cpos = rl_point;
    int epos = cpos+1;
    char c2del = rl_line_buffer[cpos];
    for (i = epos; i < strlen(rl_line_buffer); i++) {
        if (rl_line_buffer[i] == c2del) {
            epos = i;
            break;
        }
    }
    rl_delete_text(cpos, epos-cpos);
    return 0;
}

static void init_readline(void)
{
    rl_readline_name = "AkLisp";
    rl_attempted_completion_function = (rl_completion_func_t*)akl_completion;
    rl_bind_key('(', akl_insert_rbrace);
    rl_bind_key('"', akl_insert_strterm);
    rl_bind_key('\b', akl_char_delete);
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
const static struct option akl_options[] = {
    { "assemble"   , no_argument,       0, 'a' },
    { "compile"    , no_argument,       0, 'c' },
    { "define"     , no_argument,       0, 'D' },
    { "eval"       , required_argument, 0, 'e' },
    { "interactive", no_argument,       0, 'i' },
    { "config"     , required_argument, 0, 'C' },
    { "no-colors"  , no_argument,       &no_color_flag,  1  },
    { "help"       , no_argument,       0, 'h' },
    { "version"    , no_argument,       0, 'v' },
    { NULL         ,           0,       0,  0  }
};

const char *akl_option_desc[] = {
    "Compile an AkLisp assembly file", "Compile an AkLisp program to bytecode"
    , "Define a variable from command-line", "Evaulate a command-line expression"
    , "Force interactive mode", "Pass configuration setting to akl-cfg!", "Disable colors"
    , "This help message", "Print the version number"
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

/* XXX: This should be dynamic */
char var[64]; /* Should be enough */
char value[64];
static void cmd_parse_define(const char *opt)
{
    struct akl_value *v = NULL;
    AKL_ASSERT(opt, AKL_NOTHING);
    var[0] = value[0] = '\0';
    sscanf(opt, "%63[A-Za-z_+-*]=%63[A-Za-z0-9 ]", var, value);
    akl_set_global_variable(&state, var, FALSE, NULL, FALSE, akl_new_string_value(&state, value));
    printf("define %s as %s\n", var, value);
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
    printf("Copyleft (C) 2014 Akos Kovacs\n\n");
    AKL_SET_FEATURE(&state, AKL_CFG_INTERACTIVE);

#if ALWAYS_DUMP_INSTR
    AKL_SET_FEATURE(&state, AKL_DEBUG_INSTR);
#endif // ALWAYS_DUMP_INSTR

#if ALWAYS_DUMP_STACK
    AKL_SET_FEATURE(&state, AKL_DEBUG_STACK);
#endif // ALWAYS_DUMP_INSTR

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

            if (AKL_IS_FEATURE_ON(&state, AKL_DEBUG_INSTR)) {
                akl_dump_ir(ctx, state.ai_fn_main);
            }
            akl_execute(ctx);
//            akl_clear_ir(ctx);

            if (AKL_IS_FEATURE_ON(&state, AKL_DEBUG_STACK)) {
                akl_dump_stack(ctx);
            }
            printf(" => ");
            akl_print_value(&state, akl_stack_pop(ctx));
            akl_print_errors(&state);
            akl_clear_errors(&state);
            akl_clear_ir(ctx);
            printf("\n");
#if HAVE_READLINE
            free(line);
#endif //  HAVE_READLINE
        }
        lnum++;
    }
}

int main(int argc, char* const* argv)
{
    FILE *fp;
    int c;
    int opt_index = 1;
    struct akl_io_device *dev;
    struct akl_context *ctx;
    struct akl_list args;
    const char *fname;

    akl_init_state(&state, NULL);
    akl_init_list(&args);
    akl_init_library(&state, AKL_LIB_ALL);

#ifdef HAVE_GETOPT_H
    while((c = getopt_long(argc, argv, "aD:C:e:chiv", akl_options, &opt_index)) != -1) {
        if (no_color_flag)
            AKL_UNSET_FEATURE(&state, AKL_CFG_USE_COLORS);

        switch (c) {
            case 'a':
            printf("Assembling\n");
            return 0;

            case 'e':
            printf("Eval this line \'%s\'\n", optarg);
            return 0;

            case 'h':
            print_help();
            return 0;

            case 'v':
            printf("AkLisp v%d.%d-%s\n", VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
            return 0;

            case 'D':
                cmd_parse_define(optarg);
            break;

            case 'C':
                akl_set_feature(&state, optarg);
            break;

            case 'i':
            default:
            interactive_mode();
            break;
        }
    }
#endif // HAVE_GETOPT_H
    if (opt_index < argc) {
        fname = argv[opt_index];
        fp = fopen(fname, "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", fname);
            return -1;
        }
        while (opt_index < argc) {
            akl_list_append_value(&state, &args
                , akl_new_string_value(&state, strdup(argv[opt_index++])));
        }
        AKL_UNSET_FEATURE(&state, AKL_CFG_INTERACTIVE);

        akl_set_global_variable(&state, AKL_CSTR("*args*"), AKL_CSTR("The list of command-line arguments")
            , akl_new_list_value(&state, &args));

        akl_set_global_variable(&state, AKL_CSTR("*file*"), AKL_CSTR("The current file")
            , akl_new_string_value(&state, strdup(fname)));

        dev = akl_new_file_device(&state, fname, fp);
        ctx = akl_compile(&state, dev);
        akl_execute(ctx);
    } else {
        interactive_mode();
        return 0;
    }

    return 0;
}
