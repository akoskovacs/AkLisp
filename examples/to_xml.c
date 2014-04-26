#include <stdio.h>
#include "../src/aklisp.h"

struct akl_state s; // W00t global variable

void print_tabs(int n)
{
    while (n--) {
        printf("    ");
    }
}

void list_eval(int level, struct akl_list *l)
{
    struct akl_value *v;
    struct akl_atom *at, *tag = NULL;
    struct akl_list *ln;
    struct akl_list_iterator *it;
    const char *fmt;
    if (!l)
        return;

    it = akl_list_begin(&s, l);
    print_tabs(level);

    while (akl_list_has_next(it)) {
        v = (struct akl_value *)akl_list_next(it);
        switch (AKL_TYPE(v)) {
            case TYPE_NUMBER:
            printf("%f ", AKL_GET_NUMBER_VALUE(v));
            break;

            case TYPE_STRING:
            if (akl_list_count(l) == 1)
                fmt = "%s ";
            else
                fmt = "\"%s\" ";

            printf(fmt, AKL_GET_STRING_VALUE(v));
            break;

            case TYPE_ATOM:
            at = AKL_GET_ATOM_VALUE(v);
            if (AKL_IS_QUOTED(v)) {
                printf("%s=", at->at_name);    
            } else {
                tag = at;
                printf("<%s ", at->at_name);
            }

            break;

            case TYPE_LIST:
            printf("\b>\n");
            ln = AKL_GET_LIST_VALUE(v);
            list_eval(level+1, ln);
            break;

            default:
            printf(" "); // Dunno what is it...
            break;
        }
    }
    if (tag) {
        printf("\n");
        print_tabs(level);
        printf("</%s>", tag->at_name);
    }
}

int main(int argc, char **argv) {
    struct akl_list *l;
    const char *line = 
    "(html :lang \"hu\" (head :bgcolor \"white\" (title (\"hello\")))))";

    if (argc > 1)
        line = argv[1];

    akl_init_state(&s, NULL);
    l = akl_str_to_list(&s, line);
    list_eval(0, l);
        printf("\n");
    return 0;
}
