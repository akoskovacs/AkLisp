#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "aklisp.h"

AKL_CFUN_DEFINE(plus, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int sum = 0;
    char *str = NULL;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
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

AKL_CFUN_DEFINE(minus, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int ret = 0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
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

AKL_CFUN_DEFINE(times, in, args)
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
            ret *= *akl_get_number_value(times_function(in
                                , akl_get_list_value(val)));
            break;
        }
    }
    return akl_new_number_value(in, ret);
}

AKL_CFUN_DEFINE(div, in, args)
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

AKL_CFUN_DEFINE(mod, in, args)
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

AKL_CFUN_DEFINE(not, in __unused, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(and, in, args)
{
    struct akl_value *arg;
    struct akl_list_entry *ent;
    if (args->li_elem_count <= 0)
        return &TRUE_VALUE;

    AKL_LIST_FOREACH(ent, args) {
        arg = akl_entry_to_value(ent);
        ent->le_value = akl_eval_value(in, arg);
        if (AKL_IS_NIL(ent->le_value)) 
            return &NIL_VALUE;
    }
    return akl_entry_to_value(args->li_last);
}

AKL_BUILTIN_DEFINE(or, in, args)
{
    struct akl_value *arg;
    struct akl_list_entry *ent;
    if (args->li_elem_count <= 0)
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
        arg = akl_entry_to_value(ent);
        ent->le_value = akl_eval_value(in, arg);
        if (!AKL_IS_NIL(ent->le_value)) 
            return ent->le_value;
    }
    return &NIL_VALUE;
}


AKL_CFUN_DEFINE(exit, in __unused, args)
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

AKL_CFUN_DEFINE(print, in, args)
{
    print_list(args);
    return akl_new_list_value(in, args);
}

AKL_CFUN_DEFINE(display, in, args)
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


AKL_CFUN_DEFINE(car, in __unused, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
        return akl_car(a1->va_value.list);
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(cdr, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
       return akl_new_list_value(in, akl_cdr(in, a1->va_value.list));
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(len, in, args)
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

AKL_BUILTIN_DEFINE(setq, in, args)
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
        if (a->at_value->va_type == TYPE_CFUN 
                || a->at_value->va_type == TYPE_BUILTIN) {
            if (a->at_desc != NULL)
                printf("%s - %s\n", a->at_name, a->at_desc);
            else
                printf("%s - [no documentation]\n", a->at_name);
        }
    }
}

AKL_CFUN_DEFINE(help, in, args __unused)
{
   akl_do_on_all_atoms(in, print_help);
   return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(quote, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 != NULL && a1->va_type == TYPE_LIST) {
        a1->is_quoted = TRUE;
        if (a1->va_value.list != NULL)
            a1->va_value.list->is_quoted = TRUE;
    }
    return a1;
}

AKL_CFUN_DEFINE(list, in, args)
{ return quote_builtin(in, args); }

AKL_CFUN_DEFINE(version, in, args __unused)
{
    struct akl_list *version = akl_new_list(in);
    akl_list_append(in, version, akl_new_atom_value(in, "VERSION"));
    akl_list_append(in, version, akl_new_number_value(in, VER_MAJOR));
    akl_list_append(in, version, akl_new_number_value(in, VER_MINOR));
    version->is_quoted = 1;
    return akl_new_list_value(in, version);
}

AKL_CFUN_DEFINE(about, in, args)
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

AKL_CFUN_DEFINE(range, in, args)
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

AKL_CFUN_DEFINE(zerop, in, args)
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

AKL_CFUN_DEFINE(index, in, args)
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

static struct akl_value *cmp_two_args(struct akl_instance *in __unused,
                                struct akl_list *args, int cmp_op)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (akl_compare_values(a1, a2) == cmp_op)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_CFUN_DEFINE(greaterp, in, args)
{
    return cmp_two_args(in, args, 1);
}

AKL_CFUN_DEFINE(equal, in, args)
{
    return cmp_two_args(in, args, 0);
}

AKL_CFUN_DEFINE(lessp, in, args)
{
    return cmp_two_args(in, args, -1);
}


/* Evaulate the first expression (mostly a list with a logical function),
    if that is not NIL, return with the evaulated second argument. */
static struct akl_value *eval_if_true(struct akl_instance *in
                                            , struct akl_list *l)
{
    struct akl_value *a1;
    a1 = akl_eval_value(in, AKL_FIRST_VALUE(l));
    if (a1 != NULL && !AKL_IS_NIL(a1))
        return akl_eval_value(in, AKL_SECOND_VALUE(l));

    return NULL;
}

AKL_BUILTIN_DEFINE(if, in, args)
{
    struct akl_value *ret;
    if ((ret = eval_if_true(in, args)) == NULL)
        return akl_eval_value(in, AKL_THIRD_VALUE(args));

    return ret;
}

AKL_BUILTIN_DEFINE(cond, in, args)
{
    struct akl_value *arg, *ret;
    struct akl_list_entry *ent;
    AKL_LIST_FOREACH(ent, args) {
        arg = akl_entry_to_value(ent);
        if (arg != NULL && arg->va_type == TYPE_LIST) {
            ret = eval_if_true(in, akl_get_list_value(arg));
            if (ret != NULL)
                return ret;
        }
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(nilp, in, args)
{
    struct akl_value *a1;
    a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_CFUN_DEFINE(typeof, in, args)
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

AKL_CFUN_DEFINE(cons, in, args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);

    if (a1 == NULL || a2 == NULL || a2->va_type != TYPE_LIST)
        return &NIL_VALUE;
   
    akl_list_insert_head(in, akl_get_list_value(a2), a1);
    return a2;
}

AKL_CFUN_DEFINE(read_number, in, args)
{
    int num = 0;
    scanf("%d", &num);
    return akl_new_number_value(in, num);
}
#ifdef _GNUC_
AKL_CFUN_DEFINE(read_string, in, args __unused)
{
#define _GNU_SOURCE
    char *str = NULL;
    scanf("%a%s", &str);
    if (str != NULL)
        return akl_new_string_value(in, str);
}
#else // _GNUC_
AKL_CFUN_DEFINE(read_string, in, args __unused)
{
    char *str = (char *)malloc(256);
    scanf("%s", str);
    if (str != NULL)
        return akl_new_string_value(in, str);
}
#endif // _GNUC_

AKL_CFUN_DEFINE(newline, in __unused, args __unused)
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

AKL_CFUN_DEFINE(date_year, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_year + 1900);
}

AKL_CFUN_DEFINE(time_hour, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_hour);
}

AKL_CFUN_DEFINE(time_min, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_min);
}

AKL_CFUN_DEFINE(time_sec, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_sec);
}

AKL_CFUN_DEFINE(time, in, args)
{
    struct akl_list *tlist;
    tlist = akl_new_list(in);
    akl_list_append(in, tlist, time_hour_function(in, args));
    akl_list_append(in, tlist, time_min_function(in, args));
    akl_list_append(in, tlist, time_sec_function(in, args));
    return akl_new_list_value(in, tlist);
}

AKL_CFUN_DEFINE(progn, in, args)
{
    struct akl_list_entry *lent = args->li_last;
    if (lent != NULL && lent->le_value != NULL)
        return akl_entry_to_value(lent);
    else
        return &NIL_VALUE;
}

void akl_init_lib(struct akl_instance *in, enum AKL_INIT_FLAGS flags)
{
    if (flags & AKL_LIB_BASIC) {
        AKL_ADD_BUILTIN(in, quote, "QUOTE", "Quote listame like as \'");
        AKL_ADD_BUILTIN(in, setq,  "SETQ", "Bound (set) a variable to a value");
        AKL_ADD_CFUN(in, progn,"PROGN", "Return with the last value");
        AKL_ADD_CFUN(in, list, "LIST", "Create list");
        AKL_ADD_CFUN(in, car,  "CAR", "Get the head of a list");
        AKL_ADD_CFUN(in, cdr,  "CDR", "Get the tail of a list");
    }

    if (flags & AKL_LIB_DATA) {
        AKL_ADD_CFUN(in, range, "RANGE", "Create list with elements from arg 1 to arg 2");
        AKL_ADD_CFUN(in, cons,  "CONS", "Insert the first argument to the second list argument");
        AKL_ADD_CFUN(in, len,   "LENGTH", "The length of a given value");
        AKL_ADD_CFUN(in, index, "INDEX", "Index of list");
        AKL_ADD_CFUN(in, typeof,"TYPEOF", "Get the type of the value");
    }

    if (flags & AKL_LIB_CONDITIONAL) {
        AKL_ADD_BUILTIN(in, if, "IF"
        , "If the first argument true, returns with the second, otherwise returns with the third");
        AKL_ADD_BUILTIN(in, cond, "COND"
        , "Similiar to if, but works with arbitrary number of conditions (called clauses)");
    }

    if (flags & AKL_LIB_PREDICATE) {
        AKL_ADD_CFUN(in, equal, "=", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, lessp, "<",  "If it\'s first argument is less than the second returns with T");
        AKL_ADD_CFUN(in, greaterp, ">", "If it\'s first argument is greater than the second returns with T");
        AKL_ADD_CFUN(in, equal, "EQ", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, equal, "EQUAL", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, equal, "EQUALP", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, lessp, "LESSP",  "If it\'s first argument is less than the second returns with T");
        AKL_ADD_CFUN(in, greaterp, "GREATERP", "If it\'s first argument is greater than the second returns with T");
        AKL_ADD_CFUN(in, zerop, "ZEROP", "Predicate which, returns with true if it\'s argument is zero");
        AKL_ADD_CFUN(in, nilp, "NILP", "Predicate which, returns with true when it\'s argument is NIL");
    }

    if (flags & AKL_LIB_NUMBERIC) {
        AKL_ADD_CFUN(in, plus,  "+", "Arithmetic addition and string concatenation");
        AKL_ADD_CFUN(in, minus, "-", "Artihmetic subtraction");
        AKL_ADD_CFUN(in, times, "*", "Arithmetic multiplication");
        AKL_ADD_CFUN(in, div,   "/",  "Arithmetic division");
        AKL_ADD_CFUN(in, mod,   "%", "Arithmetic modulus");
        /* Ok. Again... */
        AKL_ADD_CFUN(in, plus,  "PLUS", "Arithmetic addition and string concatenation");
        AKL_ADD_CFUN(in, minus, "MINUS", "Artihmetic substraction");
        AKL_ADD_CFUN(in, times, "TIMES", "Arithmetic multiplication");
        AKL_ADD_CFUN(in, div,   "DIV",  "Arithmetic division");
        AKL_ADD_CFUN(in, mod,   "MOD", "Arithmetic modulus");
    }

    if (flags & AKL_LIB_LOGICAL) {
        AKL_ADD_CFUN(in, not, "NOT", "Reverse the argument\'s value");
        AKL_ADD_BUILTIN(in, and, "AND", "Evaluate arguments from right to left, returning NIL if"
            " any of them is NIL, otherwise returning with the last value");
        AKL_ADD_BUILTIN(in, or, "OR", "Evaluate arguments from right to left, returning the first"
            " non-NIL value, otherwise just NIL");
    }
        
    if (flags & AKL_LIB_SYSTEM) {
        AKL_ADD_CFUN(in, read_number, "READ-NUMBER", "Read a number from the standard input");
        AKL_ADD_CFUN(in, read_string, "READ-STRING", "Read a string from the standard input");
        AKL_ADD_CFUN(in, newline, "NEWLINE", "Just put out a newline character to the standard output");
        AKL_ADD_CFUN(in, print,   "PRINT", "Print different values to the screen in Lisp-style");
        AKL_ADD_CFUN(in, display, "DISPLAY", "Print numbers or strings to the screen");
        AKL_ADD_CFUN(in, help,    "HELP", "Print the builtin functions");
        AKL_ADD_CFUN(in, about,   "ABOUT", "About the environment");
        AKL_ADD_CFUN(in, version, "VERSION", "About the version");
        AKL_ADD_CFUN(in, exit, "QUIT", "Exit from the program");
        AKL_ADD_CFUN(in, exit, "EXIT", "Exit from the program");
    }

    if (flags & AKL_LIB_TIME) {
        AKL_ADD_CFUN(in, date_year, "DATE-YEAR", "Get the current year");
        AKL_ADD_CFUN(in, time_hour, "TIME-HOUR", "Get the current hour");
        AKL_ADD_CFUN(in, time_min,  "TIME-MIN", "Get the current minute");
        AKL_ADD_CFUN(in, time_sec,  "TIME-SEC", "Get the current secundum");
        AKL_ADD_CFUN(in, time, "TIME", "Get the current time as a list of (hour minute secundum)");
    }

    if (flags & AKL_LIB_OS) {
        akl_init_os(in);
    }
}
