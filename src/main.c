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
static struct akl_state in;

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>

/* Give back a possible completion of 'text', by traversing
 through the red black tree of all global atoms. */
static char *
akl_generator(const char *text, int state)
{
    static struct akl_atom *atom;
    static size_t tlen = 0;
    /* If this is the first run, initialize the 'atom' with
      the first element of the red black tree. */
    if (!state) {
        atom = RB_MIN(ATOM_TREE, &in->ai_atom_head);
        tlen = strlen(text);
    } else {
        atom = RB_NEXT(ATOM_TREE, &in->ai_atom_head, atom);
    }

    while (atom != NULL) {
        if (strncasecmp(atom->at_name, text, tlen) == 0)
            return strdup(atom->at_name);

        atom = RB_NEXT(ATOM_TREE, &in->ai_atom_head, atom);
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

static void interactive_mode(void)
{
    char prompt[PROMPT_MAX];
    int lnum = 1;
    struct akl_value *value;
    char *line;
    printf("Interactive AkLisp version %d.%d-%s\n"
        , VER_MAJOR, VER_MINOR, VER_ADDITIONAL);
    printf("Copyleft (C) 2012 Akos Kovacs\n\n");
    akl_init_state(&in);
    in.ai_interactive = TRUE;
    akl_init_lib(&in, AKL_LIB_ALL);
    init_readline();
    while (1) {
        snprintf(prompt, PROMPT_MAX, "[%d]> ", lnum);
        line = readline(prompt);
        if (line == NULL || strcmp(line, "exit") == 0) {
            printf("Bye!\n");
            AKL_FREE(line);
            return;
        }
        if (line && *line) {
            add_history(line);
            value = akl_parse_string(&in, "stdin", line);
            akl_print_value(in, value);
            /*akl_list_append(in, inst->ai_program, il);*/
            printf("\n => ");
            akl_print_value(&in, akl_eval_value(&in, value));
            akl_print_errors(&in);
            akl_clear_errors(&in);
            printf("\n");
        }
        lnum++;
    }
}

int main(int argc, const char *argv[])
{
    FILE *fp;
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", argv[1]);
            return -1;
        }
        // TODO...
        in = akl_new_file_interpreter(argv[1], fp);
        in->ai_interactive = FALSE;
    } else {
        interactive_mode();
        return 0;
    }
    akl_init_lib(in, AKL_LIB_ALL);
    akl_parse(in);
    akl_eval_program(in);
    akl_print_errors(in);
    akl_clear_errors(in);
//    akl_free_state(in);
    return 0;
}
