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
#include <stdlib.h>
#include "aklisp.h"

struct akl_value TRUE_VALUE = {
    .va_type = TYPE_TRUE,
    .va_value.number = 1,
    .is_quoted = TRUE,
    .is_nil = FALSE,
};

struct akl_value NIL_VALUE = {
    .va_type = TYPE_NIL,
    .va_value.number = 0,
    .is_quoted = TRUE,
    .is_nil = TRUE,
};

struct akl_list NIL_LIST = {
    .li_head = NULL,
    .li_last = NULL,
    .li_elem_count = 0,
    .is_nil = TRUE,
    .li_locals = NULL,
    .li_local_count = 0,
};

void *akl_malloc(struct akl_instance *in, size_t size)
{
    void *ptr;
    ptr = MALLOC_FUNCTION(size);
    if (in)
        in->ai_gc_stat[AKL_GC_STAT_ALLOC] += size;
    if (ptr == NULL) {
        fprintf(stderr, "ERROR! No memory left!\n");
        exit(-1);
    } else {
        return ptr;
    }
}

void akl_gc_value_destruct(struct akl_instance *in, void *obj)
{
    struct akl_value *val = (struct akl_value *)obj;
    /* We can simply compare just the pointers */
    if (val != &NIL_VALUE && val != &TRUE_VALUE)
        akl_free_value(in, val);
}

void akl_gc_list_destruct(struct akl_instance *in, void *obj)
{
    struct akl_list *list = (struct akl_list *)obj;
    if (list != &NIL_LIST)
        akl_free_list(in, list);
}

void akl_gc_atom_destruct(struct akl_instance *in, void *obj)
{
    akl_free_atom(in, (struct akl_atom *)obj);
}

struct akl_instance * 
akl_new_file_interpreter(FILE *fp)
{
    struct akl_instance *in = akl_new_instance();
    in->ai_device = akl_new_file_device(fp);
    return in;
}

struct akl_instance *
akl_new_string_interpreter(const char *str)
{
    struct akl_instance *in = akl_new_instance();
    in->ai_device = akl_new_string_device(str);
    return in;
}

struct akl_instance *
akl_reset_string_interpreter(struct akl_instance *in, const char *str)
{
   if (in == NULL) {
       return akl_new_string_interpreter(str);
   } else if (in->ai_device == NULL) {
       in->ai_device = akl_new_string_device(str);
       return in;
   } else {
       in->ai_device->iod_type = DEVICE_STRING;
       in->ai_device->iod_source.string = str;
       in->ai_device->iod_pos        = 0;
       in->ai_device->iod_char_count = 0;
       in->ai_device->iod_line_count = 0;
       return in;
   }
}

void akl_free_instance(struct akl_instance *in)
{
    struct akl_atom *t1, *t2;
#if 0
    RB_FOREACH_SAFE(t1, ATOM_TREE, &in->ai_atom_head, t2) {
        ATOM_TREE_RB_REMOVE(t1, &in->ai_atom_head);
        akl_free_atom(in, t1);
    }
    akl_free_list(in, in->ai_program);
#endif
}

struct akl_instance *akl_new_instance(void)
{
    struct akl_instance *in = AKL_MALLOC(NULL, struct akl_instance);
    RB_INIT(&in->ai_atom_head);
    AKL_GC_INIT_OBJ(&NIL_VALUE, akl_gc_value_destruct);
    AKL_GC_INIT_OBJ(&TRUE_VALUE, akl_gc_value_destruct);
    AKL_GC_INIT_OBJ(&NIL_LIST, akl_gc_list_destruct);
    memset(in->ai_gc_stat, 0, AKL_NR_GC_STAT_ENT * sizeof(unsigned int));
    in->ai_program  = NULL;
    return in;
}

struct akl_list *akl_new_list(struct akl_instance *in)
{
    struct akl_list *lh = AKL_MALLOC(in, struct akl_list);
    AKL_GC_INIT_OBJ(lh, akl_gc_list_destruct);
    lh->li_head   = NULL;
    lh->li_last   = NULL;
    lh->is_quoted = FALSE;
    lh->is_nil    = FALSE;
    lh->li_locals = NULL;
    lh->li_parent = NULL;
    lh->li_elem_count  = 0;
    lh->li_local_count = 0;
    in && in->ai_gc_stat[AKL_GC_STAT_LIST]++;

    return lh;
}

struct akl_atom *akl_new_atom(struct akl_instance *in, char *name)
{
    struct akl_atom *atom = AKL_MALLOC(in, struct akl_atom);
    AKL_GC_INIT_OBJ(atom, akl_gc_atom_destruct);
    assert(name);
    atom->at_value = NULL;
    atom->at_name = name;
    atom->at_desc = NULL;
    in && in->ai_gc_stat[AKL_GC_STAT_ATOM]++;
    return atom;
}

struct akl_list_entry *akl_new_list_entry(struct akl_instance *in)
{
    struct akl_list_entry *ent = AKL_MALLOC(in, struct akl_list_entry);
    ent->le_value = NULL;
    ent->le_next = NULL;
    in && in->ai_gc_stat[AKL_GC_STAT_LIST_ENTRY]++;
    return ent;
}

void akl_free_list_entry(struct akl_instance *in, struct akl_list_entry *ent)
{
    if (ent == NULL)
        return;

    AKL_GC_DEC_REF(in, AKL_ENTRY_VALUE(ent));
    in && in->ai_gc_stat[AKL_GC_STAT_LIST_ENTRY]--;
    AKL_FREE(ent);
}

struct akl_value *akl_new_value(struct akl_instance *in)
{
    struct akl_value *val = AKL_MALLOC(in, struct akl_value);
    AKL_GC_INIT_OBJ(val, akl_gc_value_destruct);
    val->is_nil    = FALSE;
    val->is_quoted = FALSE;
    return val;
}

void akl_free_value(struct akl_instance *in, struct akl_value *val)
{
    if (val == NULL)
        return;

    switch (val->va_type) {
        case TYPE_LIST:
            AKL_GC_DEC_REF(in, val->va_value.list);
            break;

        case TYPE_ATOM:
            AKL_GC_DEC_REF(in, val->va_value.atom);
            break;

        case TYPE_STRING:
            /* No need for akl_free_string_value()  */
            AKL_FREE(val->va_value.string);
            in && in->ai_gc_stat[AKL_GC_STAT_STRING]--;
            break;

        case TYPE_NUMBER:
            in && in->ai_gc_stat[AKL_GC_STAT_NUMBER]--;
            break;

        default:
        /* On NIL_* and TRUE_* values we don't need
          to decrease the reference count. */
        return;
    }
    AKL_FREE(val);
}

struct akl_value *akl_new_string_value(struct akl_instance *in, char *str)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = TYPE_STRING;
    val->va_value.string = str;
    val->is_nil = FALSE;
    in && in->ai_gc_stat[AKL_GC_STAT_STRING]++;
    return val;
}

struct akl_value *akl_new_number_value(struct akl_instance *in, double num)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = TYPE_NUMBER;
    val->va_value.number = num;
    in && in->ai_gc_stat[AKL_GC_STAT_NUMBER]++;
    return val;
}

struct akl_value *akl_new_list_value(struct akl_instance *in, struct akl_list *lh)
{
    struct akl_value *val = akl_new_value(in);
    assert(lh != NULL);
    val->va_type = TYPE_LIST;
    val->va_value.list = lh;
    AKL_GC_INC_REF(lh);
    return val;
}

void akl_free_list(struct akl_instance *in, struct akl_list *list)
{
    struct akl_list_entry *ent, *tmp;
    if (list == NULL)
        return;

    AKL_LIST_FOREACH_SAFE(ent, list, tmp) {
        akl_free_list_entry(in, ent);
    }
    in && in->ai_gc_stat[AKL_GC_STAT_LIST]--;
    AKL_FREE(list);
}

struct akl_value *
akl_new_atom_value(struct akl_instance *in, char *name)
{
    struct akl_value *val = akl_new_value(in);
    assert(name != NULL);
    struct akl_atom *atm = akl_new_atom(in, name);
    val->va_type = TYPE_ATOM;
    val->va_value.atom = atm;
    AKL_GC_INC_REF(atm);
    return val;
}

void akl_free_atom(struct akl_instance *in, struct akl_atom *atom)
{
    if (atom == NULL)
        return;
    if (atom->at_value != NULL 
        && atom->at_value->va_type != TYPE_CFUN
        && atom->at_value->va_type != TYPE_BUILTIN) {
        AKL_FREE(atom->at_name);
        AKL_FREE(atom->at_desc);
    }
    akl_free_value(in, atom->at_value);
    AKL_FREE(atom);
    in && in->ai_gc_stat[AKL_GC_STAT_ATOM]--;
}

char *akl_get_atom_name_value(struct akl_value *val)
{
    struct akl_atom *atom = AKL_GET_ATOM_VALUE(val);
    return (atom != NULL) ? atom->at_name : NULL;
}

struct akl_io_device *
akl_new_file_device(FILE *fp)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(NULL, struct akl_io_device);
    dev->iod_type = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos = 0;
    return dev;
}

struct akl_io_device *
akl_new_string_device(const char *str)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(NULL, struct akl_io_device);
    dev->iod_type = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos = 0;
    return dev;
}
