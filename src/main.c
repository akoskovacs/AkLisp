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
# include <getopt.h>
# define AKL_HISTORY_FILE "./config/AkLisp/.history"
#endif // HAVE_GETOPT_H

#define PROMPT_MAX 10
static struct akl_state state;

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>

/* Give back a possible completion of 'text', by traversing
 through the red black tree of all global symbols. */
static char *
akl_symbol_generator(const char *text, int st)
{
    static struct akl_symbol *sym = NULL;
    static size_t tlen = 0;
    /* If this is the first run, initialize the symbol with
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

/* Give back a possible completion of 'text', by traversing
 through the red black tree of all global functions. */
static char *
akl_var_generator(const char *text, int st)
{
    static struct akl_variable *var = NULL;
    static size_t tlen = 0;
    struct akl_symbol *sym;
    /* If this is the first run, initialize the symbol with
      the first element of the red black tree. */
    if (!st) {
        var = RB_MIN(VAR_TREE, &state.ai_global_vars);
        tlen = strlen(text);
    } else {
        var = RB_NEXT(VAR_TREE, &state.ai_global_vars, var);
    }

    while (var != NULL) {
        sym = var->vr_symbol;
        if (strncasecmp(sym->sb_name, text, tlen) == 0)
            return strdup(sym->sb_name);

        var = RB_NEXT(VAR_TREE, &state.ai_global_vars, var);
    }
    return NULL;
}

/* Create an array of strings, containing the possible
  completions of the given 'text', using 'akl_*_generator'.*/
static char **
akl_completion(char *text, int start, int end)
{
    char **matches = (char **)NULL;
    if (rl_line_buffer[start-1] == ':') {
        matches = rl_completion_matches(text, akl_symbol_generator);
    } else {
        matches = rl_completion_matches(text, akl_var_generator);
    }

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
    char cch, nch;
    cch = rl_line_buffer[rl_point];
    nch = rl_line_buffer[rl_point+1];
    rl_message("%c %c delete", cch, nch);
    if ((cch == '(' && nch == ')') || (cch == '"' && nch == cch)) {
        rl_delete_text(rl_point, rl_point+1);
    }
#if 0
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
#endif
    return 0;
}

static void init_readline(void)
{
    rl_readline_name = "AkLisp";
    rl_attempted_completion_function = (rl_completion_func_t*)akl_completion;
    rl_bind_key('(', akl_insert_rbrace);
    rl_bind_key('"', akl_insert_strterm);
    rl_bind_key('\b', akl_char_delete);
    using_history();
    read_history(AKL_HISTORY_FILE);
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
int eval_flag;
int peval_flag;
int compile_flag;
int force_interact_flag;
const static struct option akl_options[] = {
    { "assemble"   , no_argument,       0, 'a' },
    { "compile"    , no_argument,       &compile_flag, 'c' },
    { "define"     , no_argument,       0, 'D' },
    { "debug"      , no_argument,       0, 'd' },
    { "eval"       , required_argument, &eval_flag,  'e' },
    { "peval"      , required_argument, &peval_flag, 'E' },
    { "interactive", no_argument,       &force_interact_flag, 'i' },
    { "config"     , required_argument, 0, 'C' },
    { "no-colors"  , no_argument,       &no_color_flag,  1  },
    { "help"       , no_argument,       0, 'h' },
    { "version"    , no_argument,       0, 'v' },
    { NULL         ,           0,       0,  0  }
};

const char *akl_option_desc[] = {
    "Compile an AkLisp assembly file", "Compile an AkLisp program to bytecode"
    , "Define a variable from command-line", "Set self debugging on (stack and instructions)"
    , "Evaluate a command-line expression", "Evaluate a command-line expression and print the result"
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
void eval_dev(struct akl_io_device *dev)
{
    struct akl_context *ctx;
    ctx = akl_compile(&state, dev);
    if (AKL_IS_FEATURE_ON(&state, AKL_DEBUG_INSTR)) {
        akl_dump_ir(ctx, ctx->cx_fn_main);
    }
    akl_execute(ctx);
    if (AKL_IS_FEATURE_ON(&state, AKL_CFG_INTERACTIVE)) {
        printf(" => ");
    }
    /* Only print the evaluated expression, when interactive or -E is used */
    if (AKL_IS_FEATURE_ON(&state, AKL_CFG_INTERACTIVE) || peval_flag) {
        akl_print_value(&state, akl_stack_pop(ctx));
        if (peval_flag) {
            printf("\n");
        }
    }
    if (AKL_IS_FEATURE_ON(&state, AKL_DEBUG_STACK)) {
        akl_dump_stack(ctx);
    }
    akl_print_errors(&state);
    akl_clear_errors(&state);
    akl_clear_ir(ctx);
}

static void interactive_mode(void)
{
    char prompt[PROMPT_MAX];
    int lnum = 1;
    char *line;
    struct akl_io_device *dev;
    printf("Interactive AkLisp version %d.%d-%s\n"
        , VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
    printf("Copyleft (C) 2016 Akos Kovacs\n\n");
    AKL_SET_FEATURE(&state, AKL_CFG_INTERACTIVE);

    init_readline();
    while (1) {
        snprintf(prompt, PROMPT_MAX, "[%d]> ", lnum);
            /*AKL_GRAY, lnum, AKL_END_COLOR_MARK);*/

        line = readline(prompt);
        if (line == NULL || strcmp(line, "exit") == 0) {
            printf("Bye!\n");
            write_history(AKL_HISTORY_FILE);
            free(line);
            return;
        }
        if (line && *line) {
            add_history(line);
            dev = akl_new_string_device(&state, "stdio", line);
            eval_dev(dev);
            printf("\n");
#if HAVE_READLINE
            free(line);
#endif //  HAVE_READLINE
        }
        lnum++;
    }
}

void init_aklisp(void)
{
    /* Use normal allocation functions */
    akl_init_state(&state, NULL);
    /* Include every function */
    akl_init_library(&state, AKL_LIB_ALL);
}

int main(int argc, char* const* argv)
{
    FILE *fp;
    int c;
    int opt_index = 1;
    struct akl_io_device *dev = NULL;
    struct akl_list args;
    struct akl_value *args_value = AKL_NIL, *file_value = AKL_NIL;
    const char *fname = NULL, *eval_arg = NULL;

    init_aklisp();
    akl_init_list(&args);

#ifdef HAVE_GETOPT_H
    while ((c = getopt_long(argc, argv, "aD:dC:e:E:chiv", akl_options, &opt_index)) != -1) {
        if (no_color_flag)
            AKL_UNSET_FEATURE(&state, AKL_CFG_USE_COLORS);

        switch (c) {
            case 'a':
            printf("Assembling\n");
            return 0;

            case 'h':
            print_help();
            return 0;

            case 'E':
            peval_flag = 1; /* getopt_long() does not want to set this */
            case 'e':
            eval_arg = optarg;
            break;

            case 'v':
            printf("AkLisp v%d.%d-%s\n", VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
            return 0;

            case 'D':
            cmd_parse_define(optarg);
            break;

            case 'C':
            akl_set_feature(&state, optarg);
            break;

            case 'd':
            AKL_SET_FEATURE(&state, AKL_DEBUG_INSTR);
            AKL_SET_FEATURE(&state, AKL_DEBUG_STACK);
            break;

            default:
            break;
        }
    }
#endif // HAVE_GETOPT_H
    if (optind < argc) {
        fname = argv[optind];

        while (optind < argc) {
            akl_list_append_value(&state, &args
                , akl_new_string_value(&state, strdup(argv[optind++])));
        }
    }

    /* If there are no other arguments, the *args* will be NIL */
    if (akl_list_count(&args) > 0) {
            args_value = akl_new_list_value(&state, &args);
    }
    akl_set_global_variable(&state, AKL_CSTR("*args*")
        , AKL_CSTR("The list of command-line arguments"), args_value);

    /* Sometimes is good to have an short way */
    akl_set_global_variable(&state, AKL_CSTR("*argc*")
        , AKL_CSTR("Count of the command-line arguments '(length *args*)'")
        , akl_new_number_value(&state, akl_list_count(&args)));

    if (eval_arg) {
        dev = akl_new_string_device(&state, "eval", eval_arg);
    } else if (fname != NULL) {
        fp = fopen(fname, "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", fname);
            return -1;
        }
        dev = akl_new_file_device(&state, fname, fp);
        file_value = akl_new_string_value(&state, strdup(fname));
    }
    akl_set_global_variable(&state, AKL_CSTR("*file*")
        , AKL_CSTR("The current file"), file_value);

    if (dev != NULL) {
        AKL_UNSET_FEATURE(&state, AKL_CFG_INTERACTIVE);
        eval_dev(dev);
#if 0
        if (force_interact_flag) {
            init_aklisp(); /* Must reinitialize the interpreter */
            interactive_mode();
        }
#endif
    } else {
        interactive_mode();
    }
    return 0;
}
