#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(list))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, list) {
       val = AKL_ENTRY_VALUE(ent);
       switch (val->va_type) {
            case TYPE_NUMBER:
            if (is_first) {
                ret = *akl_get_number_value(val);
                is_first = FALSE;
            } else {
                ret -= *akl_get_number_value(val);
            }
            break;
#if 0
            case TYPE_STRING:
            /* TODO */
            break;

            case TYPE_LIST:
            /* TODO: Handle string lists */
            ret += *akl_get_number_value(minus_function(in
                 , akl_get_list_value(val)));
            break;
#endif
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
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
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
    struct akl_list_entry *ent;
    struct akl_value *val;
    int ret = 0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
        if (val->va_type == TYPE_NUMBER) {
            if (is_first) {
                ret = *akl_get_number_value(val);
                is_first = FALSE;
            } else {
                ret /= *akl_get_number_value(val);
            }
       }
    }
    return akl_new_number_value(in, ret);
}

static struct akl_value *mod_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int ret = 0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
        if (val->va_type == TYPE_NUMBER) {
            if (is_first) {
                ret = *akl_get_number_value(val);
                is_first = FALSE;
            } else {
                ret %= *akl_get_number_value(val);
            }
        }
    }
    return akl_new_number_value(in, ret);
}

static struct akl_value *exit_function(struct akl_instance *in __unused
                                           , struct akl_list *args)
{
    printf("Bye!\n");
    if (AKL_IS_NIL(args)) {
        exit(0);
    } else {
        exit(*akl_get_number_value(AKL_FIRST_VALUE(args)));
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
        printf("%s%s", AKL_IS_QUOTED(val) ? ":" : ""
            , akl_get_atom_name_value(val));
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
    if (AKL_IS_NIL(list) || list == NULL) {
        printf("(NIL)");
        return;
    }

    if (AKL_IS_QUOTED(list))
        printf("\'");
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
    return akl_new_list_value(in, args);
}

static struct akl_value *display_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_list_entry *ent;
    struct akl_value *tmp;
    AKL_LIST_FOREACH(ent, args) {
        tmp = AKL_ENTRY_VALUE(ent);
        switch (tmp->va_type) {
            case TYPE_NUMBER:
            printf("%d", *akl_get_number_value(tmp));
            break;

            case TYPE_STRING:
            printf("%s", akl_get_string_value(tmp));
            break;
        }
    }
    return &NIL_VALUE;
}

static struct akl_value *car_function(struct akl_instance *in __unused
                               , struct akl_list *args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
        return akl_car(a1->va_value.list);
    }
    return &NIL_VALUE;
}

static struct akl_value *cdr_function(struct akl_instance *in
                               , struct akl_list *args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
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

        default:
        return &NIL_VALUE;
    }
    return &NIL_VALUE;
}

static struct akl_value *setq_builtin(struct akl_instance *in, struct akl_list *args)
{
    struct akl_atom *atom;
    struct akl_value *value;
    atom = akl_get_atom_value(AKL_FIRST_VALUE(args));
    if (atom == NULL) {
        fprintf(stderr, "setq: First argument is not an atom!\n");
        exit(-1);
    }
    value = akl_eval_value(in, AKL_SECOND_VALUE(args));
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

static struct akl_value *quote_builtin(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 != NULL && a1->va_type == TYPE_LIST) {
        a1->is_quoted = TRUE;
        if (a1->va_value.list != NULL)
            a1->va_value.list->is_quoted = TRUE;
    }
    return a1;
}

static struct akl_value *list_function(struct akl_instance *in, struct akl_list *args)
{ return quote_builtin(in, args); }

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
        farg = AKL_FIRST_VALUE(args);
        sarg = AKL_SECOND_VALUE(args);
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
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
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
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (a1 && !AKL_IS_NIL(a1) && a1->va_type == TYPE_NUMBER) {
        ind = *akl_get_number_value(a1);
    }
    if (a2 && !AKL_IS_NIL(a2) && a2->va_type == TYPE_LIST) {
        list = akl_get_list_value(a2);
        return akl_list_index(list, ind);
    }
    return &NIL_VALUE;
}

static struct akl_value *eq_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (akl_compare_values(a1, a2) == 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *greater_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (akl_compare_values(a1, a2) > 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *less_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (akl_compare_values(a1, a2) < 0)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *if_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1;
    a1 = akl_eval_value(in, AKL_FIRST_VALUE(args));
    if (AKL_IS_NIL(a1)) 
        return akl_eval_value(in, AKL_THIRD_VALUE(args));

    return akl_eval_value(in, AKL_SECOND_VALUE(args));
}

static struct akl_value *nilp_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1;
    a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

static struct akl_value *typeof_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *ret;
    const char *tname = "NIL";
    a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL)
        return &NIL_VALUE;

    switch (a1->va_type) {
        case TYPE_NUMBER:
        tname = "NUMBER";
        break;

        case TYPE_ATOM:
        tname = "ATOM";
        break;

        case TYPE_LIST:
        tname = "LIST";
        break;

        case TYPE_CFUN:
        tname = "CFUNCTION";
        break;

        case TYPE_STRING:
        tname = "STRING";
        break;

        case TYPE_BUILTIN:
        tname = "BUILTIN";
        break;

        case TYPE_TRUE:
        tname = "T";
        break;

        case TYPE_NIL:
        tname = "NIL";
        break;
    }
    /* Must duplicate the name for gc */
    ret = akl_new_atom_value(in, strdup(tname));
    ret->is_quoted = TRUE;
    return ret;
}

struct akl_value *cons_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);

    if (a1 == NULL || a2 == NULL || a2->va_type != TYPE_LIST)
        return &NIL_VALUE;
   
    akl_list_insert_head(in, akl_get_list_value(a2), a1);
    return a2;
}

struct akl_value *read_number_function(struct akl_instance *in, struct akl_list *args)
{
    int num = 0;
    scanf("%d", &num);
    return akl_new_number_value(in, num);
}
#ifdef _GNUC_
struct akl_value *read_string_function(struct akl_instance *in, struct akl_list *args __unused)
{
#define _GNU_SOURCE
    char *str = NULL;
    scanf("%a%s", &str);
    if (str != NULL)
        return akl_new_string_value(in, str);
}
#else
struct akl_value *read_string_function(struct akl_instance *in, struct akl_list *args __unused)
{
    char *str = (char *)malloc(256);
    scanf("%s", str);
    if (str != NULL)
        return akl_new_string_value(in, str);
}
#endif

struct akl_value *newline_function(struct akl_instance *i __unused, struct akl_list *a __unused)
{
    printf("\n");
    return &NIL_VALUE;
}

static struct tm *akl_time(void)
{
    time_t curr;
    time(&curr);
    return localtime(&curr);
}

struct akl_value *date_year_function(struct akl_instance *in, struct akl_list *args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_year + 1900);
}

struct akl_value *time_hour_function(struct akl_instance *in, struct akl_list *args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_hour);
}

struct akl_value *time_min_function(struct akl_instance *in, struct akl_list *args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_min);
}

struct akl_value *time_sec_function(struct akl_instance *in, struct akl_list *args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_sec);
}

struct akl_value *time_function(struct akl_instance *in, struct akl_list *args)
{
    struct akl_list *tlist;
    tlist = akl_new_list(in);
    akl_list_append(in, tlist, time_hour_function(in, args));
    akl_list_append(in, tlist, time_min_function(in, args));
    akl_list_append(in, tlist, time_sec_function(in, args));
    return akl_new_list_value(in, tlist);
}

void akl_init_lib(struct akl_instance *in, enum AKL_INIT_FLAGS flags)
{
    if (flags & AKL_LIB_BASIC) {
        akl_add_global_cfun(in, "LIST", list_function, "Create list");
        akl_add_builtin(in, "QUOTE", quote_builtin, "Quote list, same as \'");
        akl_add_builtin(in, "SETQ", setq_builtin, "Bound (set) a variable to a value");
        akl_add_global_cfun(in, "CAR", car_function, "Get the head of a list");
        akl_add_global_cfun(in, "CDR", cdr_function, "Get the tail of a list");
    }
    if (flags & AKL_LIB_DATA) {
        akl_add_global_cfun(in, "RANGE", range_function
            , "Create list with elements from arg 1 to arg 2");
        akl_add_global_cfun(in, "CONS", cons_function
            , "Insert the first argument to the second list argument");
        akl_add_global_cfun(in, "LENGTH", len_function, "The length of a given value");
        akl_add_global_cfun(in, "INDEX", index_function, "Index of list");
        akl_add_global_cfun(in, "TYPEOF", typeof_function, "Get the type of the value");
    }
    if (flags & AKL_LIB_CONDITIONAL) {
        akl_add_builtin(in, "IF", if_function
            , "If the first argument true, returns with the second, otherwise returns with the third");
    }
    if (flags & AKL_LIB_PREDICATE) {
        akl_add_global_cfun(in, "=", eq_function
            , "Tests it\'s argument for equality");
        akl_add_global_cfun(in, "<", less_function
            , "If it\'s first argument is less than the second returns with T");
        akl_add_global_cfun(in, ">", greater_function
            , "If it\'s first argument is greater than the second returns with T");
        akl_add_global_cfun(in, "EQ", eq_function
            , "Tests it\'s argument for equality");
        akl_add_global_cfun(in, "LT", less_function
            , "If it\'s first argument is less than the second returns with T");
        akl_add_global_cfun(in, "GT", greater_function
            , "If it\'s first argument is greater than the second returns with T");
        akl_add_global_cfun(in, "LESSP", less_function
            , "If it\'s first argument is less than the second returns with T");
        akl_add_global_cfun(in, "GREATERP", less_function
            , "If it\'s first argument is less than the second returns with T");
        akl_add_global_cfun(in, "ZEROP", zerop_function
            , "Predicate which, returns with true if it\'s argument is zero");
    }

    if (flags & AKL_LIB_NUMBERIC) {
        akl_add_global_cfun(in, "+", plus_function, "Arithmetic addition and string concatenation");
        akl_add_global_cfun(in, "-", minus_function, "Artihmetic minus");
        akl_add_global_cfun(in, "*", mul_function
            , "Arithmetic multiplication and string \'multiplication\'");
        akl_add_global_cfun(in, "/", div_function, "Arithmetic devide");
        akl_add_global_cfun(in, "%", mod_function, "Arithmetic modulus");
        /* Ok. Again... */
        akl_add_global_cfun(in, "ADD", plus_function, "Arithmetic addition and string concatenation");
        akl_add_global_cfun(in, "MINUS", minus_function, "Artihmetic minus");
        akl_add_global_cfun(in, "MUL", mul_function
            , "Arithmetic multiplication and string \'multiplication\'");
        akl_add_global_cfun(in, "DIV", div_function, "Arithmetic devide");
        akl_add_global_cfun(in, "MOD", mod_function, "Arithmetic modulus");
    }
    
    if (flags & AKL_LIB_SYSTEM) {
        akl_add_global_cfun(in, "READ-NUMBER", read_number_function, "Read a number from the standard input");
        akl_add_global_cfun(in, "READ-STRING", read_string_function, "Read a string from the standard input");
        akl_add_global_cfun(in, "NEWLINE", newline_function, "Just put out a newline character to the standard output");
        akl_add_global_cfun(in, "PRINT", print_function, "Print different values to the screen in Lisp-style");
        akl_add_global_cfun(in, "DISPLAY", display_function, "Print numbers or strings to the screen");
        akl_add_global_cfun(in, "HELP", help_function, "Print the builtin functions");
        akl_add_global_cfun(in, "ABOUT", about_function, "About the environment");
        akl_add_global_cfun(in, "VERSION", version_function, "About the version");
        akl_add_global_cfun(in, "QUIT", exit_function, "Exit from the program");
        akl_add_global_cfun(in, "EXIT", exit_function, "Exit from the program");
    }

    if (flags & AKL_LIB_TIME) {
        akl_add_global_cfun(in, "DATE-YEAR", date_year_function, "Get the current year");
        akl_add_global_cfun(in, "TIME", time_function, "Get the current time as a list of (hour minute secundum)");
        akl_add_global_cfun(in, "TIME-HOUR", time_hour_function, "Get the current hour");
        akl_add_global_cfun(in, "TIME-MIN", time_min_function, "Get the current minute");
        akl_add_global_cfun(in, "TIME-SEC", time_sec_function, "Get the current secundum");
    }
}
