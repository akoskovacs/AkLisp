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

void akl_add_global_atom(struct akl_instance *in, struct akl_atom *atom)
{
    AKL_INC_REF(in, atom);
    ATOM_TREE_RB_INSERT(&in->ai_atom_head, atom);
}

struct akl_atom *
akl_add_global_cfun(struct akl_instance *in, const char *name
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
akl_add_builtin(struct akl_instance *in, const char *name
        , akl_cfun_t fn, const char *desc)
{
    assert(name);
    assert(fn);
    struct akl_atom *atom = akl_add_global_cfun(in, name, fn, desc);
    atom->at_value->va_type = TYPE_BUILTIN;
    return atom;
}

struct akl_atom *
akl_get_global_atom(struct akl_instance *in, const char *name)
{
    struct akl_atom *atm, *res;
    if (name == NULL)
        return NULL;

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
    AKL_INC_REF(in, val);
    return le; 
}

struct akl_list_entry *
akl_list_insert_head(struct akl_instance *in, struct akl_list *list, struct akl_value *val)
{
    struct akl_list_entry *le;
    struct akl_list_entry *head;
    le = akl_new_list_entry(in);
    le->le_value = val;
    if (list->li_head == NULL) {
        list->li_last = le;
    } else {
        head = list->li_head;
        le->le_next = head;
    }
    list->li_head = le;
    list->li_elem_count++;
    AKL_INC_REF(in, val);
    return le;
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
        /* Yeah! Extremely inefficient! */
        return akl_list_index(list, list->li_elem_count + index);
    } else {
        ent = list->li_head;
        while (index--) {
            if ((ent = AKL_LIST_NEXT(ent)) == NULL)
                return &NIL_VALUE;
        }
        val = AKL_ENTRY_VALUE(ent);
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

void akl_print_value(struct akl_value *val)
{
    if (AKL_IS_NIL(val) || val == NULL) {
        printf("NIL");
    }

    switch (val->va_type) {
        case TYPE_NUMBER:
        START_COLOR(YELLOW);
        printf("%d", AKL_GET_NUMBER_VALUE(val));
        END_COLOR;
        break;

        case TYPE_STRING:
        START_COLOR(GREEN);
        printf("\"%s\"", AKL_GET_STRING_VALUE(val));
        END_COLOR;
        break;

        case TYPE_LIST:
        akl_print_list(AKL_GET_LIST_VALUE(val));
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

        case TYPE_NIL:
        START_COLOR(GRAY);
        printf("NIL");
        END_COLOR;
        break;
    }
}

void akl_print_list(struct akl_list *list)
{
    struct akl_list_entry *ent;
    
    assert(list);
    if (AKL_IS_NIL(list) || list == NULL 
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
        akl_print_value(AKL_ENTRY_VALUE(ent));
        if (ent->le_next != NULL)
            printf(" ");
    }
    printf(")");
}
