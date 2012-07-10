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

struct akl_value *akl_eval_value(struct akl_instance *in, struct akl_value *val)
{
    struct akl_atom *aval;
    char *fname;
    if (AKL_IS_QUOTED(val) || val == NULL) {
        return val;
    }

    switch (val->va_type) {
        case TYPE_ATOM:
        aval = akl_get_atom_value(val);
        if (aval->at_value != NULL)
           return aval->at_value;
        fname = aval->at_name;
        aval = akl_get_global_atom(in, fname);
        if (aval != NULL && aval->at_value != NULL) {
           return aval->at_value;
        } else {
           fprintf(stderr, "ERROR: No value for \'%s\' atom!\n"
                , fname);
           exit(-1);
        }
        break;

        case TYPE_LIST:
        return akl_eval_list(in, akl_get_list_value(val));
        break;
    }
}

struct akl_value *akl_eval_list(struct akl_instance *in, struct akl_list *list)
{
    akl_cfun_t cfun;
    struct akl_list *args;
    struct akl_atom *fatm, *aval;
    struct akl_list_entry *ent;
    struct akl_value *ret, *tmp;
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
            if (tmp->va_type == TYPE_LIST || tmp->va_type == TYPE_ATOM
                    && is_mutable == FALSE)
                ent->le_value = akl_eval_value(in, AKL_ENTRY_VALUE(ent));
        }
    }

        
    if (list->li_elem_count > 1)
        args = akl_cdr(in, list);
    else 
        args = &NIL_LIST;

    assert(args);
    assert(cfun);
    ret = cfun(in, args);
//    akl_free_list(in, list);
//    AKL_FREE(args);
    return ret;
}

struct akl_list_entry *
akl_list_append(struct akl_instance *in, struct akl_list *list, struct akl_value *val)
{
    struct akl_list_entry *le;
    assert(list != NULL);
    assert(val != NULL);

    le = akl_new_list_entry(in);
    le->le_value = val;

    if (list->li_head == NULL) {
        list->li_head = le;
    } else {
        list->li_last->le_next = le; 
    }

    list->li_last = le;
    list->li_elem_count++;
    list->is_nil = 0;
    return le; 
}

int akl_compare_values(struct akl_value *v1, struct akl_value *v2)
{
    int n1, n2;
    if (v1->va_type == v2->va_type) {
        switch (v1->va_type) {
            case TYPE_NUMBER:
            n1 = *akl_get_number_value(v1);
            n2 = *akl_get_number_value(v2);
            if (n1 == n2)
                return 0;
            else if (n1 > n2)
                return 1;
            else
                return -1;
            break;

            case TYPE_STRING:
            return strcmp(akl_get_string_value(v1)
                          , akl_get_string_value(v2));
            break;

            case TYPE_ATOM:
            return strcasecmp(akl_get_atom_name_value(v1)
                          , akl_get_atom_name_value(v2));
            break;
        }
    }
    return -1;
}

struct akl_list_entry *akl_list_find(struct akl_list *list, struct akl_value *val)
{
    struct akl_list_entry *ent;
    struct akl_value *v;
    AKL_LIST_FOREACH(ent, list) {
       v = AKL_ENTRY_VALUE(ent);
       if (akl_compare_values(v, val) == 0)
           return ent;
    }
    return NULL;
}

struct akl_value *akl_list_index(struct akl_list *list, int index)
{
    struct akl_value *val = &NIL_VALUE;
    struct akl_list_entry *ent;
    if (list == NULL || list->li_head == NULL || AKL_IS_NIL(list))
        return val;
    if (index < 0) {
        /* TODO: Implement reverse foreach */
    } else {
        ent = AKL_LIST_FIRST(list);
        while (index--) {
            if ((ent = AKL_LIST_NEXT(ent)) == NULL)
                return &NIL_VALUE;

        }
        val = akl_entry_to_value(ent);
    }
    return val;
}

bool_t akl_list_remove(struct akl_instance *in, struct akl_list *list
                       , struct akl_value *val)
{
    struct akl_list_entry *ent;
    struct akl_value *v;
    if (list == NULL || val == NULL)
        return FALSE;

    v = akl_entry_to_value(list->li_head);
    if (v != NULL && akl_compare_values(val, v)) {
        list->li_head = AKL_LIST_SECOND(list);
    }
    AKL_LIST_FOREACH(ent, list) {
       v = akl_entry_to_value(ent->le_next);
       if (v != NULL && akl_compare_values(val, v)) {
           /* TODO: Free the entry */
           ent = ent->le_next;
           return TRUE;
       }
    }
    return FALSE;
}

struct akl_value *akl_entry_to_value(struct akl_list_entry *ent)
{
    if (ent != NULL) {
        return ent->le_value;
    }
    return NULL;
}

struct akl_value *akl_car(struct akl_list *l)
{
    return AKL_ENTRY_VALUE(AKL_LIST_FIRST(l));
}

struct akl_list *akl_cdr(struct akl_instance *in, struct akl_list *l)
{
    struct akl_list *nhead;
    assert(l);
    if (AKL_IS_NIL(l))
        return &NIL_LIST;

    nhead = akl_new_list(in);
    nhead->li_elem_count = l->li_elem_count - 1;
    if (nhead->li_elem_count <= 0) {
        nhead->li_head = nhead->li_last = NULL;
        nhead->is_nil = 1;
    } else {
        nhead->li_head = l->li_head->le_next;
        nhead->li_last = l->li_last;
    }
    return nhead;
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
