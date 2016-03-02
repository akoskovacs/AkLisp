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
    struct akl_symbol *sym;
    //struct akl_atom *at, *tag = NULL;
    struct akl_list *ln;
    struct akl_list_entry *it;
    const char *fmt;
    if (!l)
        return;

    it = akl_list_it_begin(l);
    print_tabs(level);

    while (akl_list_it_has_next(it)) {
        v = (struct akl_value *)akl_list_it_next(&it);
        switch (AKL_TYPE(v)) {
            case AKL_VT_NUMBER:
            printf("%f ", AKL_GET_NUMBER_VALUE(v));
            break;

            case AKL_VT_STRING:
            if (akl_list_count(l) == 1)
                fmt = "%s ";
            else
                fmt = "\"%s\" ";

            printf(fmt, AKL_GET_STRING_VALUE(v));
            break;

            case AKL_VT_SYMBOL:
            sym = v->va_value.symbol;
            printf("%s=", sym->sb_name);    
            break;

            case AKL_VT_LIST:
            printf("\b>\n");
            ln = AKL_GET_LIST_VALUE(v);
            list_eval(level+1, ln);
            break;

            default:
            printf(" "); // Dunno what is it...
            break;
        }
    }
    #if 0
    if (tag) {
        printf("\n");
        print_tabs(level);
        printf("</%s>", tag->at_name);
    }
    #endif
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
