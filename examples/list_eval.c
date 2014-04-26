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
    struct akl_atom *at;
    struct akl_list *ln;
    struct akl_list_iterator *it;
    if (!l)
        return;

    it = akl_list_begin(&s, l);
    print_tabs(level);     printf("Got list: ");
    akl_print_list(&s, l); printf("\n");

    while (akl_list_has_next(it)) {
        print_tabs(level);
        v = (struct akl_value *)akl_list_next(it);
        switch (AKL_TYPE(v)) {
            case TYPE_NUMBER:
            printf("Got number: ");
            akl_print_value(&s, v);
            break;

            case TYPE_STRING:
            printf("Got string: ");
            akl_print_value(&s, v);
            break;

            case TYPE_ATOM:
            if (AKL_IS_QUOTED(v))
                printf("Got symbol: ");
            else
                printf("Got atom: ");
            akl_print_value(&s, v);
            break;

            case TYPE_TRUE:
            printf("Got true");
            break;

            case TYPE_NIL:
            printf("Got nil");
            break;

            case TYPE_LIST:
            printf("Recurse!!!\n");
            ln = AKL_GET_LIST_VALUE(v);
            list_eval(level+1, ln);
            break;

            default:
            printf("Dunno what is it...");
            break;
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    struct akl_list *l;
    const char *line = "(this \"is\" 1 'cool :list (and another))";

    if (argc > 1)
        line = argv[1];

    akl_init_state(&s, NULL);
    l = akl_str_to_list(&s, line);
    list_eval(0, l);
    return 0;
}
