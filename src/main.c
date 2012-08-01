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
#define HAVE_READLINE

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <unistd.h>
#include <string.h>

#define PROMPT_MAX 10
static struct akl_instance *in = NULL;

static char *
akl_generator(const char *text, int state)
{
    static struct akl_atom *atom;
    static size_t tlen = 0;
    char *ret = NULL;
    if (!state) {
        atom = RB_MIN(ATOM_TREE, &in->ai_atom_head);
        tlen = strlen(text);
    }

    while (atom != NULL) {
        if (strncasecmp(atom->at_name, text, tlen) == 0)
            ret = strdup(atom->at_name);

        atom = RB_NEXT(ATOM_TREE, &in->ai_atom_head, atom);
        if (ret)
            return ret;
    }
    return NULL;
}

static char **
akl_completition(char *text, int start, int end)
{
    char **matches = (char **)NULL;
    if (rl_line_buffer[start-1] == '(')
        matches = rl_completion_matches(text, akl_generator);

    return matches;
}

static int akl_insert_rbrace(int count, int key)
{
    rl_insert_text("(");
    rl_insert_text(")");
    rl_backward_char(1, 1);
    return 0;
}

static void init_readline(void)
{
    rl_readline_name = "AkLisp";
    rl_attempted_completion_function = (CPPFunction *)akl_completition;
    rl_bind_key('(', akl_insert_rbrace);
}

static void interactive_mode(void)
{
    char prompt[PROMPT_MAX];
    int lnum = 1;
    struct akl_list *il;
    char *line;
    printf("Interactive AkLisp version %d.%d-%s\n"
        , VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
    printf("Copyleft (C) 2012 Akos Kovacs\n\n");
    in = akl_new_instance();
    akl_init_lib(in, AKL_LIB_ALL);
    init_readline();
    while (1) {
        snprintf(prompt, PROMPT_MAX, "[%d]> ", lnum);
        line = readline(prompt);
        if (line == NULL || strcmp(line, "exit") == 0) {
            printf("Bye!\n");
            free(line);
            return;
        }
        if (line && *line) {
            add_history(line);
            in->ai_device = akl_new_string_device(line);
            /*inst->ai_program = akl_new_list(in);*/
            il = akl_parse_list(in, in->ai_device, FALSE);
            il = AKL_GET_LIST_VALUE(AKL_FIRST_VALUE(il));
            akl_print_list(il);
            /*akl_list_append(in, inst->ai_program, il);*/
            printf("\n => ");
            akl_print_value(akl_eval_list(in, il));
            printf("\n");
            free(in->ai_device);
        }
        lnum++;
    }
}

int main(int argc, const char *argv[])
{
    FILE *fp;
    struct akl_instance *inst;
    struct akl_list *list;
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", argv[1]);
            return -1;
        }
        inst = akl_new_file_interpreter(fp);
    } else {
        interactive_mode();
        return 0;
    }
    akl_init_lib(inst, AKL_LIB_ALL);
    akl_parse_io(inst);
    akl_eval_program(inst);
    akl_free_instance(inst);
    return 0;
}
