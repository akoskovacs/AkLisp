#include "aklisp.h"

RB_GENERATE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

void akl_add_global_atom(struct akl_instance *in, struct akl_atom *atom)
{
    ATOM_TREE_RB_INSERT(&in->ai_atom_head, atom);
}

struct akl_atom *
akl_add_global_cfun(struct akl_instance *in, const char *name
        , akl_cfun_t fn, const char *desc)
{
    assert(name);
    struct akl_atom *atom = akl_new_atom(in, (char *)name);
    if (desc != NULL)
        atom->at_desc = (char *)desc;
    atom->at_value = akl_new_value(in);
    atom->at_value->va_type = TYPE_CFUN;
    atom->at_value->va_value.cfunc = fn;
    akl_add_global_atom(in, atom);
    return atom;
}

struct akl_atom *
akl_get_global_atom(struct akl_instance *in, const char *name)
{
    struct akl_atom *atm, *res;
    atm = akl_new_atom(in, strdup(name));
    res = ATOM_TREE_RB_FIND(&in->ai_atom_head, atm);
    akl_free_atom(in, atm);
    return res;
}

void akl_do_on_all_atoms(struct akl_instance *in, void (*fn) (struct akl_atom *))
{
    struct akl_atom *atm;
    RB_FOREACH(atm, ATOM_TREE, &in->ai_atom_head) {
       fn(atm);
    }
}

akl_cfun_t akl_get_global_cfun(struct akl_instance *in, const char *name)
{
    struct akl_atom *atm = akl_get_global_atom(in, name);
    if (atm != NULL && name != NULL && atm->at_value != NULL) {
        if (atm->at_value->va_type == TYPE_CFUN)
            return atm->at_value->va_value.cfunc;
    }
    return NULL;
}

struct akl_value *akl_eval_list(struct akl_instance *in, struct akl_list *list)
{
    akl_cfun_t cfun;
    struct akl_list *args;
    struct akl_atom *fatm, *aval;
    struct akl_list_entry *ent;
    struct akl_value *tmp, *ret;
    bool_t is_mutable = FALSE;
    assert(list);

    if (AKL_IS_NIL(list) || list->li_elem_count == 0) 
        return &NIL_VALUE;

    if (AKL_IS_QUOTED(list)) {
        ret = akl_new_list_value(in, list);
        ret->is_quoted = 1;
        return ret;
    }
    
    assert(AKL_LIST_FIRST(list)->le_value);
    assert(AKL_LIST_FIRST(list)->le_value->va_type == TYPE_ATOM);
    fatm = akl_get_atom_value(AKL_LIST_FIRST(list)->le_value);
    assert(fatm);
    cfun = akl_get_global_cfun(in, fatm->at_name);
    if (cfun == NULL) {
        fprintf(stderr, "ERROR: Cannot find \'%s\' function!\n"
            , fatm->at_name);
        exit(-1);
    }

    if ((strncmp("SET", fatm->at_name, 3) == 0)
            || (strncmp("DEF", fatm->at_name, 3) == 0))
        is_mutable = TRUE;

    if (list->li_elem_count > 1) {
        /* Not quoted, so start the list processing 
            from the second element. */
        for (ent = AKL_LIST_SECOND(list); ent; ent = AKL_LIST_NEXT(ent)) {
            tmp = AKL_ENTRY_VALUE(ent);
            if (AKL_IS_QUOTED(tmp)) {
                continue;
            }

            switch (tmp->va_type) {
                case TYPE_ATOM:
                   aval = akl_get_atom_value(tmp); 
                   if (is_mutable) 
                       continue;

                   if (aval->at_value != NULL)
                       ent->le_value = aval->at_value;
                   aval = akl_get_global_atom(in, aval->at_name);
                   if (aval != NULL && aval->at_value != NULL) {
                       ent->le_value = aval->at_value;
                   } else {
                       fprintf(stderr, "ERROR: No value for \'%s\' atom!\n"
                            , aval->at_name);
                       exit(-1);
                   }
                break;

                case TYPE_LIST:
                    ent->le_value = akl_eval_list(in
                        , akl_get_list_value(tmp));
                break;
            }
        }
    }

        
    if (list->li_elem_count > 1)
        args = akl_cdr(in, list);
    else 
        args = &NIL_LIST;

    assert(args);
    assert(cfun);
    ret = cfun(in, args);
    akl_free_list(in, list);
//    AKL_FREE(args);
    return ret;
}

void akl_eval_program(struct akl_instance *in)
{
    struct akl_list *plist = in->ai_program;
    struct akl_list_entry *ent;
    struct akl_value *val, *ret;
    if (plist != NULL) {
        AKL_LIST_FOREACH(ent, plist) {
            val = AKL_ENTRY_VALUE(ent);
            if (val->va_type == TYPE_LIST) {
                ret = akl_eval_list(in, val->va_value.list);
                assert(ret);
                printf(" => ");
                print_value(ret);
                printf("\n");
            }
        }
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
    } else {
        fp = stdin;
    }
    inst = akl_new_file_interpreter(fp);

    init_lib(inst);
    list = akl_parse_io(inst);
    akl_eval_program(inst);
    akl_free_instance(inst);
    return 0;
}
