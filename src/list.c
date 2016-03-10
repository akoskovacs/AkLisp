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
/* This function also works on sublists */
struct akl_list_entry 
*akl_list_append(struct akl_state *s, struct akl_list *list, void *data)
{
    assert(list != NULL);
    struct akl_list_entry *ent = akl_new_list_entry(s);
    ent->le_data = data;

    if (list->li_head == NULL) {
        list->li_head = ent;
    } else {
        list->li_last->le_next = ent;
        ent->le_prev = list->li_last;
    }

    list->li_last = ent;
    list->li_count++;
    list->is_nil = FALSE;
    return ent; 
}

struct akl_list_entry
*akl_list_append_value(struct akl_state *s, struct akl_list *list, struct akl_value *val)
{
    struct akl_list_entry *ent = akl_list_append(s, list, (void *)val);
    ent->gc_obj.gc_le_is_obj = TRUE;
    return ent;
}

struct akl_list_entry *
akl_list_insert_head(struct akl_state *s, struct akl_list *list, void *data)
{
    assert(list);
    struct akl_list_entry *ent = akl_new_list_entry(s);
    ent->le_data = data;
    if (list->li_head == NULL) {
        list->li_last = ent;
    } else {
        list->li_head->le_prev = ent;
        ent->le_next = list->li_head; 
    }
    list->li_head = ent;
    list->li_count++;
    list->is_nil = FALSE;
    return ent;
}

void *akl_list_head(struct akl_list *list)
{
    if (list != NULL) {
        return (list->li_head == NULL) ? NULL : list->li_head->le_data;
    }
    return NULL;
}

struct akl_list_entry *
akl_list_insert_head_value(struct akl_state *s, struct akl_list *list, struct akl_value *val)
{
    struct akl_list_entry *ent = akl_list_insert_head(s, list, (void *)val);
    ent->gc_obj.gc_le_is_obj = TRUE;
    return ent;
}

struct akl_list_entry *
akl_list_shift_entry(struct akl_list *list)
{
    assert(list);
    struct akl_list_entry *ohead = list->li_head;
    struct akl_list_entry *nhead = (ohead) ? ohead->le_next : NULL;
    list->li_head = nhead;
    if (nhead)
        nhead->le_prev = NULL;

    if (ohead == list->li_last)
        list->li_last = nhead;

    list->li_count--;
    /* TODO: Can explicitly free() the ohead */
    return ohead;
}

void *
akl_list_shift(struct akl_list *list)
{
    struct akl_list_entry *ent = akl_list_shift_entry(list);
    return ent ? ent->le_data : NULL;
}

struct akl_value *akl_duplicate_value(struct akl_state *in, struct akl_value *oval)
{
    struct akl_value *nval;
    if (oval == NULL)
        return NULL;

    switch (oval->va_type) {
        case AKL_VT_LIST:
        return akl_new_list_value(in
                  , akl_list_duplicate(in, AKL_GET_LIST_VALUE(oval)));

        case AKL_VT_SYMBOL:
        nval = akl_new_value(in);
        nval->va_type         = AKL_VT_SYMBOL;
        /* The symbol itself, never duplicated. */
        nval->va_value.symbol = oval->va_value.symbol;
        return nval;

        case AKL_VT_NUMBER:
        return akl_new_number_value(in, AKL_GET_NUMBER_VALUE(oval));

        case AKL_VT_STRING:
        return akl_new_string_value(in, AKL_GET_STRING_VALUE(oval));

        case AKL_VT_FUNCTION:
        nval = akl_new_value(in);
        *nval = *oval;
        return nval;

        case AKL_VT_USERDATA:
        /* TODO: Should provide specific copy function */
        return akl_new_user_value(in, akl_get_utype_value(oval)
                                 , akl_get_userdata_value(oval)->ud_private);

        case AKL_VT_NIL:
        return &NIL_VALUE;

        case AKL_VT_TRUE:
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

struct akl_list_entry *
akl_list_find(struct akl_list *list, akl_cmp_fn_t cmp_fn, void *data, unsigned int *ind)
{
    assert(list);
    assert(cmp_fn);
    struct akl_list_entry *ent;
    void *ptr;
    unsigned int i = 0;
    AKL_LIST_FOREACH(ent, list) {
       if (cmp_fn(ent->le_data, data) == 0)
           if (ind != NULL)
               *ind = i;
           return ent;
       i++;
    }
    return NULL;
}

struct akl_list_entry *
akl_list_find_value(struct akl_list *list, struct akl_value *val, unsigned int *ind)
{
    return akl_list_find(list, akl_compare_values, val, ind);
}

struct akl_list_entry *
akl_list_index_entry(struct akl_list *list, int index)
{
    assert(list && !AKL_IS_NIL(list));
    struct akl_list_entry *ent;

    if (list->li_head == NULL)
        return NULL;

    if (index >= 0) {
        ent = list->li_head;
        while (index--) {
            if ((ent = AKL_LIST_NEXT(ent)) == NULL)
                return NULL;
        }
    } else {
        ent = list->li_last;
        while (++index) {
            if ((ent = AKL_LIST_PREV(ent)) == NULL)
                return NULL;
        }
    }
    return ent;
}

void *akl_list_index(struct akl_list *list, int index)
{
    struct akl_list_entry *ent = akl_list_index_entry(list, index);
    return ent ? ent->le_data : NULL;
}

struct akl_value *
akl_list_index_value(struct akl_list *list, int index)
{
    return (struct akl_value *)akl_list_index(list, index);
}

void *akl_list_last(struct akl_list *list)
{
    if (list != NULL && list->li_last != NULL) {
        return list->li_last->le_data;
    }
    return NULL;
}

bool_t akl_list_is_empty(struct akl_list *list)
{
    return akl_list_count(list) == 0;
}

/* NOTICE: Will not free the data */
struct akl_list_entry *
akl_list_remove_entry(struct akl_list *list, struct akl_list_entry *ent)
{
    struct akl_list_entry *prev, *next;
    if (ent) {
        prev = ent->le_prev;
        next = ent->le_next;
        if (prev) {
            prev->le_next = next;
        }
        if (next) {
            next->le_prev = prev;
        }

        if (list->li_head == ent) {
            list->li_head = next;
        }

        if (list->li_last == ent) {
            list->li_last = prev;
        }
        list->li_count--;
    }
    return ent;
}

/**
 * @brief Remove an element from the list 
*/
struct akl_list_entry *
akl_list_remove(struct akl_list *list, akl_cmp_fn_t cmp_fn, void *ptr)
{
    assert(list);
    assert(cmp_fn);
    assert(ptr);
    struct akl_list_entry *ent = akl_list_find(list, cmp_fn, ptr, NULL);
    /* NOTE: The removed entry is still pointing to it's original neighbours. */
    akl_list_remove_entry(list, ent);
    return ent;
}

struct akl_list_entry *
akl_list_remove_value(struct akl_list *list, struct akl_value *val) 
{
    struct akl_list_entry *ent = akl_list_remove(list, akl_compare_values, (void *)val);
    return ent;
}

struct akl_value *akl_car(struct akl_list *l)
{
    return AKL_FIRST_VALUE(l);
}

struct akl_list_entry *
akl_list_pop_entry(struct akl_list *list)
{
    struct akl_list_entry *ent;
    if (list == NULL || list->li_last == NULL)
        return NULL;

    ent = akl_list_remove_entry(list, list->li_last);
    return ent;
}

void *akl_list_pop(struct akl_list *list)
{
    struct akl_list_entry *ent = akl_list_pop_entry(list);
    return ent ? ent->le_data : NULL;
}

unsigned int
akl_list_count(struct akl_list *l)
{
    return (l != NULL) ? l->li_count : 0;
}

struct akl_list *akl_cdr(struct akl_state *s, struct akl_list *l)
{
    struct akl_list *nhead;
    assert(l);
    if (AKL_IS_NIL(l) || l->li_count == 0)
        return NULL;

    nhead = akl_new_list(s);
    nhead->li_count = l->li_count - 1;
    if (nhead->li_count <= 0) {
        nhead->is_nil = TRUE;
    } else {
        nhead->li_head = l->li_head->le_next;
        nhead->li_last = l->li_last;
    }
    return nhead;
}

struct akl_list *
akl_list_tail(struct akl_state *s, struct akl_list *l)
{
    return akl_cdr(s, l);
}

/* Is this symbol can be found in the string array, by name? */
bool_t akl_is_strings_include(struct akl_symbol *sym, const char **strs)
{
   const char *aname = (sym != NULL) ? sym->sb_name : NULL;
   if (aname && strs) {
       while (*strs) {
           if (strcmp(aname, *strs) == 0)
               return TRUE;
           strs++;
       }
   }
   return FALSE;
}

void akl_print_value(struct akl_state *s, struct akl_value *val)
{
    struct akl_symbol *sym;
    if (val == NULL || AKL_IS_NIL(val)) {
        AKL_START_COLOR(s, AKL_BRIGHT_GRAY);
        printf("NIL");
        AKL_END_COLOR(s);
        return;
    }

    switch (val->va_type) {
        case AKL_VT_NUMBER:
        AKL_START_COLOR(s, AKL_YELLOW);
        printf("%g", AKL_GET_NUMBER_VALUE(val));
        AKL_END_COLOR(s);
        break;

        case AKL_VT_STRING:
        AKL_START_COLOR(s, AKL_GREEN);
        printf("\"%s\"", AKL_GET_STRING_VALUE(val));
        AKL_END_COLOR(s);
        break;

        case AKL_VT_LIST:
        akl_print_list(s, AKL_GET_LIST_VALUE(val));
        break;

        case AKL_VT_SYMBOL:
        sym = val->va_value.symbol;
        if (AKL_IS_QUOTED(val)) {
            AKL_START_COLOR(s, AKL_YELLOW);
            if (sym && sym->sb_name) {
                printf(":%s", sym->sb_name);
            }
        } else {
            AKL_START_COLOR(s, AKL_PURPLE);
            if (sym && sym->sb_name) {
                printf("%s", sym->sb_name);
            }
        }
        AKL_END_COLOR(s);
        break;

        case AKL_VT_TRUE:
        AKL_START_COLOR(s, AKL_BRIGHT_GREEN);
        printf("T");
        AKL_END_COLOR(s);
        break;

        case AKL_VT_USERDATA:
        AKL_START_COLOR(s, AKL_YELLOW);
        struct akl_utype *type = NULL;
        akl_utype_t tid = akl_get_utype_value(val);
        type = akl_vector_at(&s->ai_utypes, tid);
        if (type)
            printf("<USERDATA: %s>", type->ut_name);
        else
            printf("<USERDATA>");
        AKL_END_COLOR(s);
        break;

        case AKL_VT_FUNCTION:
        AKL_START_COLOR(s, AKL_YELLOW);
        printf("<FUNCTION>");
        AKL_END_COLOR(s);
        break;

        case AKL_VT_NIL:
        //case TYPE_CFUN: case TYPE_BUILTIN:
        /* Nothing to do... */
        break;
    }
}

void akl_print_list(struct akl_state *s, struct akl_list *list)
{
    struct akl_list_entry *ent;
    
    AKL_ASSERT(s, AKL_NOTHING);

    if (list == NULL || AKL_IS_NIL(list)
            || list->li_count == 0) {
        AKL_START_COLOR(s, AKL_GRAY);
        printf("NIL");
        AKL_END_COLOR(s);
        return;
    }

    if (AKL_IS_QUOTED(list))
        printf("\'");
    printf("(");
    struct akl_value *v;
    AKL_LIST_FOREACH(ent, list) {
        if (ent == NULL || ent->le_data == NULL)
            break;
        v = AKL_ENTRY_VALUE(ent);
        akl_print_value(s, v);
        if (AKL_LIST_NEXT(ent) != NULL)
            printf(" ");
    }
    printf(")");
}

struct akl_list_entry *
akl_list_it_begin(struct akl_list *l)
{
    struct akl_list_entry *ent = NULL;
    if (l) {
        ent = l->li_head;
    }
    return ent;
}

struct akl_list_entry *
akl_list_it_end(struct akl_list *l)
{
    struct akl_list_entry *ent = NULL;
    if (l) {
        ent = l->li_last;
    }
    return ent;
}

bool_t akl_list_it_has_next(struct akl_list_entry *it)
{
   return (it && it->le_next);
}

bool_t akl_list_it_has_prev(struct akl_list_entry *it)
{
   return (it && it->le_prev);
}

void *akl_list_it_next(struct akl_list_entry **it)
{
    void *p = NULL;
    if (it && (*it)) {
        p = (*it)->le_data;
        *it = (*it)->le_next;
    }
    return p;
}

void *akl_list_it_prev(struct akl_list_entry **it)
{
    void *p = NULL;
    if (it && (*it)) {
        p = (*it)->le_data;
        *it = (*it)->le_prev;
    }
    return p;
}

void *
akl_list_it_data(struct akl_list_entry *it)
{
    return (it != NULL) ? it->le_data : NULL;
}
