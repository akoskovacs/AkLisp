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

RB_GENERATE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

struct akl_atom *akl_add_global_atom(struct akl_state *in, struct akl_atom *atom)
{
    AKL_GC_SET_STATIC(atom);
    return ATOM_TREE_RB_INSERT(&in->ai_atom_head, atom);
}

void akl_remove_global_atom(struct akl_state *in, struct akl_atom *atom)
{
    ATOM_TREE_RB_REMOVE(&in->ai_atom_head, atom);
}

void akl_remove_function(struct akl_state *in, akl_cfun_t fn)
{
    struct akl_atom *atom;
    RB_FOREACH(atom, ATOM_TREE, &in->ai_atom_head) {
        if (atom->at_value && atom->at_value->va_value.cfunc == fn)
            akl_remove_global_atom(in, atom);
    }
}

struct akl_atom *
akl_add_global_cfun(struct akl_state *in, const char *name
        , akl_cfun_t fn, const char *desc)
{
    assert(name);
    assert(fn);
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
akl_add_builtin(struct akl_state *in, const char *name
        , akl_cfun_t fn, const char *desc)
{
    assert(name);
    assert(fn);
    struct akl_atom *atom = akl_add_global_cfun(in, name, fn, desc);
    atom->at_value->va_type = TYPE_BUILTIN;
    return atom;
}

struct akl_atom *
akl_get_global_atom(struct akl_state *in, const char *name)
{
    struct akl_atom *atm, *res;
    if (name == NULL)
        return NULL;

    atm = akl_new_atom(in, strdup(name));
    res = ATOM_TREE_RB_FIND(&in->ai_atom_head, atm);
//    akl_free_atom(in, atm);
    return res;
}

void akl_do_on_all_atoms(struct akl_state *in, void (*fn) (struct akl_atom *))
{
    struct akl_atom *atm;
    RB_FOREACH(atm, ATOM_TREE, &in->ai_atom_head) {
       fn(atm);
    }
}

akl_cfun_t akl_get_global_cfun(struct akl_state *in, const char *name)
{
    struct akl_atom *atm = akl_get_global_atom(in, name);
    if (atm != NULL && name != NULL && atm->at_value != NULL) {
        if (atm->at_value->va_type == TYPE_CFUN)
            return atm->at_value->va_value.cfunc;
    }
    return NULL;
}
void akl_list_append(struct akl_state *in, struct akl_list *list, akl_value *val)
{
    assert(list != NULL);
    assert(val != NULL);

    if (list->li_head == NULL) {
        list->li_head = val;
    } else {
        list->li_last->va_cdr = le;
    }

    list->li_last = val;
    list->li_elem_count++;
    list->is_nil = FALSE;
    return le; 
}

void
akl_list_insert_head(struct akl_list *list, struct akl_value *val)
{
    assert(list);
    assert(val);
    struct akl_value *head;
    if (list->li_head == NULL) {
        list->li_last = val;
    } else {
        head = list->li_head;
        val->va_cdr = head;
    }
    list->li_head = val;
    list->li_elem_count++;
}

struct akl_value *
akl_list_shift(struct akl_list *list)
{
    assert(list);
    struct akl_value *ohead = list->li_head;
    struct akl_value *nhead = (ohead) ? ohead->va_cdr : NULL;
    list->li_head = nhead;
    if (ohead == list->li_last)
        list->li_last = nhead;

    return ohead;
}

struct akl_list_entry *
akl_list_insert_value_head(struct akl_state *in, struct akl_list *list, struct akl_value *val)
{
}

struct akl_value *akl_duplicate_value(struct akl_state *in, struct akl_value *oval)
{
    struct akl_value *nval;
    struct akl_atom *natom, *oatom;
    if (oval == NULL)
        return NULL;

    switch (oval->va_type) {
        case TYPE_LIST:
        return akl_new_list_value(in
                  , akl_list_duplicate(in, AKL_GET_LIST_VALUE(oval)));

        case TYPE_ATOM:
        oatom = AKL_GET_ATOM_VALUE(oval);
        natom = akl_new_atom(in, strdup(oatom->at_name));
        natom->at_desc = strdup(oatom->at_desc);
        natom->at_value = akl_duplicate_value(in, oatom->at_value);
        return akl_new_atom_value(in, strdup(oatom->at_name));

        case TYPE_NUMBER:
        return akl_new_number_value(in, AKL_GET_NUMBER_VALUE(oval));

        case TYPE_STRING:
        return akl_new_string_value(in, AKL_GET_STRING_VALUE(oval));

        case TYPE_BUILTIN: case TYPE_CFUN:
        nval = akl_new_value(in);
        *nval = *oval;
        return nval;

        case TYPE_USERDATA:
        /* TODO: Should provide specific copy function */
        return akl_new_user_value(in, akl_get_utype_value(oval)
                                 , akl_get_userdata_value(oval)->ud_private);

        case TYPE_NIL:
        return &NIL_VALUE;

        case TYPE_TRUE:
        return &TRUE_VALUE;
    }
    return NULL;
}

struct akl_list *akl_list_duplicate(struct akl_state *in, struct akl_list *list)
{
    struct akl_list *nlist = akl_new_list(in);
    struct akl_list_entry *ent;
    struct akl_value *nval;
    AKL_LIST_FOREACH(ent, list) {
        nval = akl_duplicate_value(in, AKL_ENTRY_VALUE(ent));
        akl_list_append_value(in, nlist, nval);
    }
    return nlist;
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
    void *ptr = NULL;
    struct akl_value *ent;
    if (list == NULL || list->li_head == NULL || AKL_IS_NIL(list))
        return ptr;
    if (index < 0) {
        /* Yeah! Extremely inefficient! */
        return akl_list_index(list, list->li_elem_count + index);
    } else if (index == 0) {
        if (list->li_head)
            return list->li_head;
    } else {
        ent = list->li_head;
        while (index--) {
            if ((ent = AKL_LIST_NEXT(ent)) == NULL)
                return NULL;
        }
        ptr = ent->le_value;
    }
    return ptr;
}

struct akl_value *akl_list_index_value(struct akl_list *list, int index)
{
    struct akl_value *val = &NIL_VALUE;
    if ((val = (struct akl_value *)akl_list_index(list, index)) == NULL)
        return &NIL_VALUE;
    return val;
}

bool_t akl_list_remove_value(struct akl_state *in, struct akl_list *list
                       , struct akl_value *val)
{
    struct akl_list_entry *ent;
    struct akl_value *v;
    if (list == NULL || val == NULL)
        return FALSE;

    v = AKL_ENTRY_VALUE(list->li_head);
    if (v != NULL && akl_compare_values(val, v)) {
        list->li_head = AKL_LIST_SECOND(list);
    }
    AKL_LIST_FOREACH(ent, list) {
       v = AKL_ENTRY_VALUE(ent->le_next);
       if (v != NULL && akl_compare_values(val, v)) {
           /* TODO: Free the entry */
           ent = ent->le_next;
           return TRUE;
       }
    }
    return FALSE;
}

struct akl_value *akl_car(struct akl_list *l)
{
    return AKL_FIRST_VALUE(l);
}

struct akl_list *akl_cdr(struct akl_state *in, struct akl_list *l)
{
    struct akl_list *nhead;
    assert(l);
    if (AKL_IS_NIL(l))
        return NULL;

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

/* Is this atom name can be found in the strs? */
bool_t akl_is_equal_with(struct akl_atom *atom, const char **strs)
{
   const char *aname = (atom != NULL) ? atom->at_name : NULL;
   if (aname && strs) {
       while (*strs) {
           if (strcmp(aname, *strs) == 0)
               return TRUE;
           strs++;
       }
   }
   return FALSE;
}

void akl_print_value(struct akl_state *in, struct akl_value *val)
{
    if (val == NULL || AKL_IS_NIL(val)) {
        START_COLOR(GRAY);
        printf("NIL");
        END_COLOR;
        return;
    }

    switch (val->va_type) {
        case TYPE_NUMBER:
        START_COLOR(YELLOW);
        printf("%g", AKL_GET_NUMBER_VALUE(val));
        END_COLOR;
        break;

        case TYPE_STRING:
        START_COLOR(GREEN);
        printf("\"%s\"", AKL_GET_STRING_VALUE(val));
        END_COLOR;
        break;

        case TYPE_LIST:
        akl_print_list(in, AKL_GET_LIST_VALUE(val));
        break;

        case TYPE_ATOM:
        if (AKL_IS_QUOTED(val)) {
            START_COLOR(YELLOW);
            printf(":%s", akl_get_atom_name_value(val));
        } else {
            START_COLOR(PURPLE);
            printf("%s", akl_get_atom_name_value(val));
        }
        END_COLOR;
        break;

        case TYPE_TRUE:
        START_COLOR(BRIGHT_GREEN);
        printf("T");
        END_COLOR;
        break;

        case TYPE_USERDATA:
        START_COLOR(YELLOW);
        struct akl_utype *type = NULL;
        akl_utype_t tid = akl_get_utype_value(val);
        type = akl_vector_at(&in->ai_utypes, tid);
        if (type)
            printf("<USERDATA: %s>", type->ut_name);
        else
            printf("<USERDATA>");
        END_COLOR;
        break;

        case TYPE_NIL:
        case TYPE_CFUN: case TYPE_BUILTIN:
        /* Nothing to do... */
        break;
    }
}

void akl_print_list(struct akl_state *in, struct akl_list *list)
{
    struct akl_list_entry *ent;
    
    assert(list);
    if (list == NULL || AKL_IS_NIL(list)
        || list->li_elem_count == 0) {
        START_COLOR(GRAY);
        printf("NIL");
        END_COLOR;
        return;
    }

    if (AKL_IS_QUOTED(list)) 
        printf("\'");
    printf("(");
    AKL_LIST_FOREACH(ent, list) {
        akl_print_value(in, AKL_ENTRY_VALUE(ent));
        if (ent->le_next != NULL)
            printf(" ");
    }
    printf(")");
}
