#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "aklisp.h"

static struct akl_value *plus_function(struct akl_instance *in, struct akl_list *list)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int sum = 0;
    char *str = NULL;
    if (AKL_IS_NIL(list))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, list) {
       val = AKL_ENTRY_VALUE(ent);
       switch (val->va_type) {
            case TYPE_NUMBER:
            sum += *akl_get_number_value(val);
            break;

            case TYPE_STRING:
            if (str == NULL) {
                str = strdup(akl_get_string_value(val));
            } else {
                str = (char *)realloc(str
                  , strlen(str)+strlen(akl_get_string_value(val)));
                str = strcat(str, akl_get_string_value(val));
            }
            break;

            case TYPE_LIST:
            /* TODO: Handle string lists */
            sum += *akl_get_number_value(plus_function(in
                 , akl_get_list_value(val)));
            break;
       }
    }
    if (str == NULL)
        return akl_new_number_value(in, sum);
    else
        return akl_new_string_value(in, str);
}

static struct akl_value *minus_function(struct akl_instance *in, struct akl_list *list)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int ret = 0;
    if (AKL_IS_NIL(list))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, list) {
       val = AKL_ENTRY_VALUE(ent);
       switch (val->va_type) {
            case TYPE_NUMBER:
            ret -= *akl_get_number_value(val);
            break;

            case TYPE_STRING:
            /* TODO */
            break;

            case TYPE_LIST:
            /* TODO: Handle string lists */
            ret += *akl_get_number_value(minus_function(in
                 , akl_get_list_value(val)));
            break;
       }
    }
    return akl_new_number_value(in, ret);
}

static struct akl_value *mul_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_list_entry *ent;
    struct akl_value *val, *a1, *a2;
    int n1, i;
    char *str, *s2;
    int ret = 1;

    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    /* If the expression is something like that: (* 3 " Hello! ")
      the output will be a string containing " Hello! Hello! Hello! "*/
    a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    a2 = AKL_ENTRY_VALUE(AKL_LIST_SECOND(args));
    if (a1->va_type == TYPE_NUMBER && a2->va_type == TYPE_STRING) {
        n1 = *akl_get_number_value(a1);
        s2 = akl_get_string_value(a2);
        str = (char *)malloc((size_t)n1*strlen(s2)+1);
        for (i = 0; i < n1; i++) {
            strcat(str, s2);
        }
        return akl_new_string_value(in, str);
    }
    /* Just normal numbers and lists, get their product... */
    AKL_LIST_FOREACH(ent, args) {
        val = AKL_ENTRY_VALUE(ent);
        switch (val->va_type) {
            case TYPE_NUMBER:
            ret *= *akl_get_number_value(val);
            break;

            case TYPE_LIST:
            ret *= *akl_get_number_value(mul_function(in
                                , akl_get_list_value(val)));
            break;
        }
    }
    return akl_new_number_value(in, ret);
}

static struct akl_value *div_function(struct akl_instance *in, struct akl_list *args)
{
    /* TODO */
    return &NIL_VALUE;
}

static struct akl_value *mod_function(struct akl_instance *in, struct akl_list *args)
{
    /* TODO */
    return &NIL_VALUE;
}


static struct akl_value *getpid_function(struct akl_instance *in
                                  , struct akl_list *args __unused)
{
    return akl_new_number_value(in, (int)getpid());
}

static struct akl_value *exit_function(struct akl_instance *in __unused
                                           , struct akl_list *args)
{
    printf("Bye!\n");
    if (AKL_IS_NIL(args)) {
        exit(0);
    } else {
        exit(*akl_get_number_value(AKL_ENTRY_VALUE(AKL_LIST_FIRST(args))));
    }
}

void print_value(struct akl_value *val)
{
    if (AKL_IS_NIL(val) || val == NULL) {
        printf("NIL");
    }

    switch (val->va_type) {
        case TYPE_NUMBER:
        printf("%d", *akl_get_number_value(val));
        break;

        case TYPE_STRING:
        printf("\"%s\"", akl_get_string_value(val));
        break;

        case TYPE_LIST:
        print_list(akl_get_list_value(val));
        break;

        case TYPE_ATOM:
        printf("%s", akl_get_atom_name_value(val));
        break;

        case TYPE_TRUE:
        printf("T");
        break;
    }
}

void print_list(struct akl_list *list)
{
    struct akl_list_entry *ent;
    
    assert(list);
    if (AKL_IS_NIL(list)) {
        printf("(NIL)");
        return;
    }

    printf("(");
    AKL_LIST_FOREACH(ent, list) {
        print_value(AKL_ENTRY_VALUE(ent));
        if (ent->le_next != NULL)
            printf(" ");
    }
    printf(")");
}

static struct akl_value *print_function(struct akl_instance *in, struct akl_list *args)
{
    print_list(args);
    assert(args);
    if (AKL_IS_NIL(args)) {
        printf("NIL");
        return &NIL_VALUE;
    }
    return akl_new_list_value(in, args);
}


static struct akl_value *car_function(struct akl_instance *in __unused
                               , struct akl_list *args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    //if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
        return akl_car(a1->va_value.list);
    //}
    return &NIL_VALUE;
}

static struct akl_value *cdr_function(struct akl_instance *in
                               , struct akl_list *args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
       return akl_new_list_value(in, akl_cdr(in, a1->va_value.list));
    }
    return &NIL_VALUE;
}

static struct akl_value *len_function(struct akl_instance *in,
                               struct akl_list *args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    switch (a1->va_type) {
        case TYPE_STRING:
        return akl_new_number_value(in, strlen(akl_get_string_value(a1)));
        break;

        case TYPE_LIST:
        return akl_new_number_value(in, akl_get_list_value(a1)->li_elem_count);
        break;

        case TYPE_NUMBER:
        return akl_new_number_value(in, abs(*akl_get_number_value(a1)));
        break;

        default:
        return &NIL_VALUE;
    }
    return &NIL_VALUE;
}

static struct akl_value *setq_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_atom *atom;
    struct akl_value *value;
    atom = akl_get_atom_value(AKL_ENTRY_VALUE(AKL_LIST_FIRST(args)));
    if (atom == NULL) {
        fprintf(stderr, "setq: First argument is not an atom!\n");
        exit(-1);
    }
    value = AKL_ENTRY_VALUE(AKL_LIST_SECOND(args));
    atom->at_value = value;
    akl_add_global_atom(in, atom);
    return value;
}

void print_help(struct akl_atom *a)
{
    if (a != NULL && a->at_value != NULL) {
        if (a->at_value->va_type == TYPE_CFUN) {
            if (a->at_desc != NULL)
                printf("%s - %s\n", a->at_name, a->at_desc);
            else
                printf("%s - [no documentation]\n", a->at_name);
        }
    }
}

static struct akl_value *help_function(struct akl_instance *in
                                , struct akl_list *args __unused)
{
   akl_do_on_all_atoms(in, print_help);
   return &NIL_VALUE;
}

static struct akl_value *quote_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    if (a1 != NULL && a1->va_type == TYPE_LIST) {
        a1->is_quoted = TRUE;
        if (a1->va_value.list != NULL)
            a1->va_value.list->is_quoted = TRUE;
    }
    return a1;
}

static struct akl_value *list_function(struct akl_instance *in, struct akl_list *args)
{ return quote_function(in, args); }

static struct akl_value *version_function(struct akl_instance *in
                                   , struct akl_list *args __unused)
{
    struct akl_list *version = akl_new_list(in);
    akl_list_append(in, version, akl_new_atom_value(in, "VERSION"));
    akl_list_append(in, version, akl_new_number_value(in, VER_MAJOR));
    akl_list_append(in, version, akl_new_number_value(in, VER_MINOR));
    version->is_quoted = 1;
    return akl_new_list_value(in, version);
}

static struct akl_value *about_function(struct akl_instance *in, struct akl_list *args)
{
    printf("AkLisp version %d.%d-%s\n"
            "\tCopyleft (c) Akos Kovacs\n"
            "\tBuilt on %s %s\n\n"
            "GC Statics:\n"
            "\tATOMS: %d\n"
            "\tLISTS: %d\n"
            "\tNUMBERS: %d\n"
            "\tSTRINGS: %d\n\n"
            , VER_MAJOR, VER_MINOR, VER_ADDITIONAL
            , __DATE__, __TIME__
            , in->ai_atom_count, in->ai_list_count
           , in->ai_number_count, in->ai_string_count);

    return version_function(in, args);
}

static struct akl_value *range_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_list *range;
    struct akl_value *farg, *sarg, *targ;
    int rf, rs, rt, i;
    assert(args);
    if (args->li_elem_count > 1) {
        farg = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
        sarg = AKL_ENTRY_VALUE(AKL_LIST_SECOND(args));
    }
    if (args->li_elem_count > 2) {
        targ = AKL_LIST_SECOND(args)->le_next->le_value;
        if (targ && targ->va_type == TYPE_NUMBER)
            rt = *akl_get_number_value(targ);
        else 
            rt = 1;
    } else {
        rt = 1;
    }

    if (farg && sarg && farg->va_type == TYPE_NUMBER
            && sarg->va_type == TYPE_NUMBER) {
        range = akl_new_list(in);
        rf = *akl_get_number_value(farg);
        rs = *akl_get_number_value(sarg);
        if (rf < rs) {
            for (i = rf; i <= rs; i += rt) {
                akl_list_append(in, range, akl_new_number_value(in, i));
            }
        } else if (rf > rs) {
            for (i = rf; i >= rs; i -= rt) {
                akl_list_append(in, range, akl_new_number_value(in, i));
            }
        } else {
            akl_list_append(in, range, akl_new_number_value(in, rf));
        }
    } else {
        return &NIL_VALUE;
    }
    range->is_quoted = 1;
    return akl_new_list_value(in, range);
}

static struct akl_value *zerop_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &NIL_VALUE;
    else if (a1->va_type == TYPE_NUMBER) {
        if (a1->va_value.number == 0)
            return &TRUE_VALUE;
    } else {
        return &NIL_VALUE;
    }
    return &NIL_VALUE;
}

static struct akl_value *index_function(struct akl_instance *in, struct akl_list *args)
{
    int ind;
    struct akl_list *list;
    struct akl_value *a1, *a2;
    a1 = akl_list_index(args, 0);
    a2 = akl_list_index(args, 1);
    if (a1->va_type == TYPE_NUMBER) {
        ind = *akl_get_number_value(a1);
    }
    if (a2->va_type == TYPE_LIST) {
        list = akl_get_list_value(a2);
        return akl_list_index(list, ind);
    }
}

static struct akl_value *eq_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = akl_list_index(args, 1);
    a2 = akl_list_index(args, 2);
    if (akl_compare_values(a1, a2) == 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *greater_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = akl_list_index(args, 0);
    a2 = akl_list_index(args, 1);
    if (akl_compare_values(a1, a2) > 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *less_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = akl_list_index(args, 0);
    a2 = akl_list_index(args, 1);
    if (akl_compare_values(a1, a2) < 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *if_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2, *a3;
    a1 = akl_list_index(args, 0);
    a2 = akl_list_index(args, 1);
    a3 = akl_list_index(args, 2);
    if (a1 == NULL || a2 == NULL)
        return &NIL_VALUE;

    if (AKL_IS_NIL(a1)) {
        if (a3 != NULL)
            return a3;
        else
            return &NIL_VALUE;

    }
    return a2;
}

static struct akl_value *nilp_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1;
    a1 = akl_list_index(args, 0);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *typeof_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *ret;
    const char *typename = "NIL";
    a1 = akl_list_index(args, 0);
    if (a1 == NULL)
        return &NIL_VALUE;

    switch (a1->va_type) {
        case TYPE_NUMBER:
        typename = "NUMBER";
        break;

        case TYPE_ATOM:
        typename = "ATOM";
        break;

        case TYPE_LIST:
        typename = "LIST";
        break;

        case TYPE_CFUN:
        typename = "CFUNCTION";
        break;

        case TYPE_NIL:
        typename = "NIL";
        break;
    }
    /* Must duplicate the name for gc */
    ret = akl_new_atom_value(in, strdup(typename));
    ret->is_quoted = TRUE;
    return ret;
}

void init_lib(struct akl_instance *in)
{
    akl_add_global_cfun(in, "LIST", list_function, "Create list");
    akl_add_global_cfun(in, "QUOTE", quote_function, "Quote list, same as \'");
    akl_add_global_cfun(in, "CAR", car_function, "Get the head of a list");
    akl_add_global_cfun(in, "CDR", cdr_function, "Get the tail of a list");
    akl_add_global_cfun(in, "PRINT", print_function, "Print different values to the screen");
    akl_add_global_cfun(in, "LENGTH", len_function, "The length of a given value");
    akl_add_global_cfun(in, "SETQ", setq_function, "Bound (set) a variable to a value");
    akl_add_global_cfun(in, "HELP", help_function, "Print the builtin functions");
    akl_add_global_cfun(in, "ABOUT", about_function, "About the environment");
    akl_add_global_cfun(in, "VERSION", version_function, "About the version");
    akl_add_global_cfun(in, "RANGE", range_function, "Create list with elements from arg 1 to arg 2");
    akl_add_global_cfun(in, "ZEROP", zerop_function, "Predicate which, returns with true if it\'s argument is zero");
    akl_add_global_cfun(in, "INDEX", index_function, "Index of list");
    akl_add_global_cfun(in, "IF", if_function, "If the first argument true, returns with the second, otherwise returns with the third");
    akl_add_global_cfun(in, "TYPEOF", typeof_function, "Get the type of the value");

    akl_add_global_cfun(in, "+", plus_function, "Arithmetic addition and string concatenation");
    akl_add_global_cfun(in, "-", minus_function, "Artihmetic minus");
    akl_add_global_cfun(in, "*", mul_function, "Arithmetic multiplication and string \'multiplication\'");
    akl_add_global_cfun(in, "/", div_function, "Arithmetic devide");
    akl_add_global_cfun(in, "%", mod_function, "Arithmetic modulus");
    akl_add_global_cfun(in, "=", eq_function, "Tests it\'s argument for equality");
    akl_add_global_cfun(in, "<", less_function, "If it\'s first argument is less than the second returns with T");
    akl_add_global_cfun(in, ">", greater_function, "If it\'s first argument is greater than the second returns with T");

    /* Ok. Again... */
    akl_add_global_cfun(in, "ADD", plus_function, "Arithmetic addition and string concatenation");
    akl_add_global_cfun(in, "MINUS", minus_function, "Artihmetic minus");
    akl_add_global_cfun(in, "MUL", mul_function, "Arithmetic multiplication and string \'multiplication\'");
    akl_add_global_cfun(in, "DIV", div_function, "Arithmetic devide");
    akl_add_global_cfun(in, "MOD", mod_function, "Arithmetic modulus");
    akl_add_global_cfun(in, "EQ", eq_function, "Tests it\'s argument for equality");
    akl_add_global_cfun(in, "LT", less_function, "If it\'s first argument is less than the second returns with T");
    akl_add_global_cfun(in, "GT", greater_function, "If it\'s first argument is greater than the second returns with T");
    akl_add_global_cfun(in, "LESSP", less_function, "If it\'s first argument is less than the second returns with T");
    akl_add_global_cfun(in, "GREATERP", less_function, "If it\'s first argument is less than the second returns with T");

    akl_add_global_cfun(in, "QUIT", exit_function, "Exit from the program");
    akl_add_global_cfun(in, "EXIT", exit_function, "Exit from the program");
    akl_add_global_cfun(in, "GETPID", getpid_function, "Get the process id");

}

