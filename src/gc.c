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
    .va_lex_info = NULL,
};

struct akl_value NIL_VALUE = {
    .va_type = TYPE_NIL,
    .va_value.number = 0,
    .is_quoted = TRUE,
    .is_nil = TRUE,
    .va_lex_info = NULL,
};

void *akl_malloc(struct akl_state *in, size_t size)
{
    void *ptr;
    ptr = MALLOC_FUNCTION(size);
    if (in)
        in->ai_gc_malloc_size += size;
    if (ptr == NULL) {
        fprintf(stderr, "ERROR! No memory left!\n");
        exit(1);
    } else {
        return ptr;
    }
}

static void akl_gc_value_destruct(struct akl_state *in, void *obj)
{
    struct akl_value *val = (struct akl_value *)obj;
    if (obj == &NIL_VALUE || obj == &TRUE_VALUE)
        return;

    AKL_FREE(val->va_lex_info);
    AKL_FREE(obj);
}

static void akl_gc_normal_free_destruct(struct akl_state *in, void *obj)
{
    AKL_FREE(obj);
}

struct akl_state * 
akl_new_file_interpreter(const char *file_name, FILE *fp)
{
    struct akl_state *in = akl_new_state();
    in->ai_device = akl_new_file_device(file_name, fp);
    return in;
}

struct akl_state *
akl_new_string_interpreter(const char *name, const char *str)
{
    struct akl_state *in = akl_new_state();
    in->ai_device = akl_new_string_device(name, str);
    return in;
}

struct akl_state *
akl_reset_string_interpreter(struct akl_state *in, const char *name, const char *str)
{
   if (in == NULL) {
       return akl_new_string_interpreter(name, str);
   } else if (in->ai_device == NULL) {
       in->ai_device = akl_new_string_device(name, str);
       return in;
   } else {
       in->ai_device->iod_type = DEVICE_STRING;
       in->ai_device->iod_source.string = str;
       in->ai_device->iod_pos        = 0;
       in->ai_device->iod_char_count = 0;
       in->ai_device->iod_line_count = 0;
//       if (in->ai_program)
//           akl_free_list(in, in->ai_program);
       return in;
   }
}

/* Won't call fclose() */
struct akl_state *
akl_reset_file_interpreter(struct akl_state *in, const char *name, FILE *fp)
{
   if (in == NULL) {
       return akl_new_file_interpreter(name, fp);
   } else if (in->ai_device == NULL) {
       in->ai_device = akl_new_file_device(name, fp);
       return in;
   } else {
       in->ai_device->iod_type = DEVICE_FILE;
       in->ai_device->iod_source.string = name;
       in->ai_device->iod_pos        = 0;
       in->ai_device->iod_char_count = 0;
       in->ai_device->iod_line_count = 0;
//       if (in->ai_program)
//           akl_free_list(in, in->ai_program);
       return in;
   }
}

void akl_free_state(struct akl_state *in)
{
    struct akl_atom *t1, *t2;
#if 0
    RB_FOREACH_SAFE(t1, ATOM_TREE, &in->ai_atom_head, t2) {
        ATOM_TREE_RB_REMOVE(t1, &in->ai_atom_head);
        akl_free_atom(in, t1);
    }
    akl_free_list(in, in->ai_program);
    /* TODO: Free up user types */
#endif
    akl_clear_errors(in);
    //akl_free_list(in, in->ai_errors);
}

void akl_gc_mark_object(struct akl_gc_object *obj)
{
    assert(obj);
    if (obj->gc_generation < 3)
        obj->gc_generation++;
    return;
}

void akl_gc_mark_value(struct akl_value *v)
{
    assert(v);
    akl_gc_mark_object(&v->gc_obj);
    switch (v->va_type) {
        case TYPE_ATOM:
            akl_gc_mark_atom(v->va_value.atom);
        return;

        case TYPE_LIST:
            akl_gc_mark_list(v->va_value.list);
        return;

        default:
        return;
    }
}

void akl_gc_mark_atom(struct akl_atom *atom)
{
    assert(atom);

    akl_gc_mark_object(&atom->gc_obj);
    if (atom->at_value) {
        akl_gc_mark_value(atom->at_value);
    }
}

void akl_gc_mark_list_entry(struct akl_list_entry *le)
{
    assert(le);
    akl_gc_mark_object(&le->gc_obj);
    struct akl_value *v = le->le_value;
    if (v)
        akl_gc_mark_value(v);
}

/* NOTE: Only call with value lists! */
void akl_gc_mark_list(struct akl_list *list)
{
    struct akl_list_entry *ent;
    akl_gc_mark_object(&list->gc_obj);
    AKL_LIST_FOREACH(ent, list) {
        if (ent)
            akl_gc_mark_list_entry(ent);
    }
}

void akl_gc_mark(struct akl_state *s)
{
    struct akl_atom *atom;
    if (!s->ai_gc_is_enabled)
        return;

    RB_FOREACH(atom, ATOM_TREE, &s->ai_atom_head) {
        akl_gc_mark_atom(atom);
    }

}

void akl_gc_sweep_pool(struct akl_gc_pool *p)
{
    struct akl_vector *v;
    struct akl_gc_generic_object *go;
    akl_destructor_t defun;
    int i;
    if (!p)
        return;

    v = &p->gp_pool;
    AKL_VECTOR_FOREACH_ALL(i, go, v) {
        if (go && go->gc_obj.gc_generation == 0) {
            defun = go->gc_obj.gc_de_fun;
            if (defun)
                defun(NULL, go);

            /* Remove from the pool */
            akl_vector_remove(v, i);
        }
    }
    akl_gc_sweep_pool(p->gp_next);
}

void akl_gc_sweep(struct akl_state *s)
{
    int i;
    if (s->ai_gc_is_enabled) {
        for (i = 0; i < AKL_GC_NR_TYPE; i++) {
            akl_gc_sweep_pool(&s->ai_gc_pool[i]);
        }
    }
}

void akl_gc_enable(struct akl_state *s)
{
    if (s)
        s->ai_gc_is_enabled = TRUE;
}

void akl_gc_disable(struct akl_state *s)
{
    if (s)
        s->ai_gc_is_enabled = FALSE;
}

void akl_gc_init_pool(struct akl_state *s
                  , struct akl_gc_pool *p, enum AKL_GC_OBJECT_TYPE type)
{
    p->gp_next = NULL;
    p->gp_type = type;
    s->ai_gc_pool_count[type]++;
    s->ai_gc_pool_last[type] = p;
    akl_vector_init(&p->gp_pool, AKL_GC_POOL_SIZE, akl_gc_obj_sizes[type]);
}

struct akl_gc_pool *akl_new_gc_pool(struct akl_state *s, enum AKL_GC_OBJECT_TYPE type)
{
    struct akl_gc_pool *pool;
    pool = AKL_MALLOC(s, struct akl_gc_pool);
    akl_gc_init_pool(s, pool, type);
    return pool;
}

struct akl_function *akl_new_function(struct akl_state *s)
{
    struct akl_function *func = (struct akl_function *)akl_gc_malloc(s, AKL_GC_FUNCTION);
    func->fn_argc = 0;
    func->fn_body = NULL;
    return func;
}

void akl_gc_init(struct akl_state *s)
{
    int i;
    akl_gc_disable(s);
    for (i = 0; i < AKL_GC_NR_TYPE; i++) {
        s->ai_gc_pool_count[i] = 0;
        akl_gc_init_pool(s, &s->ai_gc_pool[i], i);
        s->ai_gc_pool_last[i] = &s->ai_gc_pool[i];
    }
}

struct akl_state *akl_new_state(void)
{
    int i;
    struct akl_state *in = AKL_MALLOC(NULL, struct akl_state);
    RB_INIT(&in->ai_atom_head);
    AKL_GC_INIT_OBJ(&NIL_VALUE, akl_gc_value_destruct);
    AKL_GC_INIT_OBJ(&TRUE_VALUE, akl_gc_value_destruct);
    AKL_GC_SET_STATIC(&NIL_VALUE);
    AKL_GC_SET_STATIC(&TRUE_VALUE);
    in->ai_device = NULL;
    akl_vector_init(&in->ai_modules, sizeof(struct akl_module *), 5);
    akl_vector_init(&in->ai_utypes, sizeof(struct akl_module *), 5);
    in->ai_program  = NULL;
    in->ai_errors   = NULL;
    akl_gc_init(in);
    return in;
}

/**
 * @brief Request memory from the GC
 * @param s An instance of the interpreter (cannot be NULL)
 * @param type Type of the GC object
 * @see AKL_GC_OBJECT_TYPE
 * If the claim is not met, try to collect some memory or 
 * create a new GC pool.
*/
void *akl_gc_malloc(struct akl_state *s, enum AKL_GC_OBJECT_TYPE type)
{
    assert(s && ptr && type < AKL_GC_NR_TYPE);
    static bool_t last_was_mark = FALSE;
    struct akl_gc_pool *p = s->ai_gc_pool_last[type];
    struct akl_vector *v = &p->gp_pool;
    if (akl_vector_is_grow_need(v)) {
        if (!last_was_mark) {
            akl_gc_mark(s);
            akl_gc_sweep(s);
            last_was_mark = TRUE;
            return akl_gc_malloc(s, type);
        } else {
            p->gp_next = akl_new_gc_pool(s, type);
            last_was_mark = FALSE;
            return akl_gc_malloc(s, type);
        }
    } else {
        return akl_vector_reserve(v);
    }
}

struct akl_list *akl_new_list(struct akl_state *s)
{
    struct akl_list *lh = (struct akl_list *)akl_gc_malloc(s, AKL_GC_LIST);
    AKL_GC_INIT_OBJ(lh, akl_gc_normal_free_destruct);
    lh->li_head   = NULL;
    lh->li_last   = NULL;
    lh->is_quoted = FALSE;
    lh->is_nil    = FALSE;
    lh->li_parent = NULL;
    lh->li_elem_count  = 0;
    lh->li_local_count = 0;
    return lh;
}

struct akl_atom *akl_new_atom(struct akl_state *s, char *name)
{
    struct akl_atom *atom = (struct akl_atom *)akl_gc_malloc(s, AKL_GC_ATOM);
    AKL_GC_INIT_OBJ(atom, akl_gc_normal_free_destruct);
    assert(name);
    atom->at_value = NULL;
    atom->at_name = name;
    atom->at_desc = NULL;
    return atom;
}

#if 0
struct akl_list_entry *akl_new_list_entry(struct akl_state *s)
{
    struct akl_list_entry *ent = AKL_MALLOC(s, struct akl_list_entry);
    AKL_GC_INIT_OBJ(ent, akl_gc_normal_free_destruct);
    ent->le_value = NULL;
    ent->le_next = NULL;
    akl_gc_pool_add(s, ent, AKL_GC_LIST_ENTRY);
    return ent;
}
#endif

struct akl_lex_info *akl_new_lex_info(struct akl_state *in, struct akl_io_device *dev)
{
    struct akl_lex_info *info = AKL_MALLOC(in, struct akl_lex_info);
    if (dev) {
        info->li_line = dev->iod_line_count;
        /* The column, where the token start */
        info->li_count = dev->iod_column;
        info->li_name = dev->iod_name;
    }
    return info;
}

struct akl_value *akl_new_value(struct akl_state *s)
{
    struct akl_value *val = (struct akl_value *)akl_gc_malloc(s, AKL_GC_VALUE);
    AKL_GC_INIT_OBJ(val, akl_gc_value_destruct);
    val->is_nil    = FALSE;
    val->is_quoted = FALSE;
    val->va_lex_info = NULL;
    val->va_cdr      = NULL;
    return val;
}

struct akl_value *akl_new_string_value(struct akl_state *in, char *str)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = TYPE_STRING;
    val->va_value.string = str;
    val->is_nil = FALSE;
    return val;
}

struct akl_value *akl_new_number_value(struct akl_state *in, double num)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = TYPE_NUMBER;
    val->va_value.number = num;
    return val;
}

struct akl_value *akl_new_list_value(struct akl_state *in, struct akl_list *lh)
{
    struct akl_value *val = akl_new_value(in);
    assert(lh != NULL);
    val->va_type = TYPE_LIST;
    val->va_value.list = lh;
    return val;
}

struct akl_value *akl_new_user_value(struct akl_state *in, unsigned int type, void *data)
{
    struct akl_userdata *udata;
    struct akl_value    *value;
    /* We should stop now, since the requested type does not exist */
    assert(akl_vector_at(&in->ai_utypes, type));
    udata = (struct akl_userdata *)akl_gc_malloc(in, AKL_GC_UDATA);
    value = akl_new_value(in);
    udata->ud_id = type;
    udata->ud_private = data;
    value->va_type = TYPE_USERDATA;
    value->va_value.udata = udata;
    return value;
}

struct akl_value *
akl_new_atom_value(struct akl_state *in, char *name)
{
    assert(name);
    struct akl_value *val = akl_new_value(in);
    struct akl_atom *atm = akl_new_atom(in, name);
    val->va_type = TYPE_ATOM;
    val->va_value.atom = atm;
    return val;
}

struct akl_io_device *
akl_new_file_device(const char *file_name, FILE *fp)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(NULL, struct akl_io_device);
    dev->iod_type = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos = 0;
    dev->iod_line_count = 1;
    dev->iod_name = file_name;
    dev->iod_buffer = NULL;
    dev->iod_buffer_size = 0;
    akl_vector_init(&dev->iod_tokens, 60, sizeof(akl_token_t));
    return dev;
}

struct akl_io_device *
akl_new_string_device(const char *name, const char *str)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(NULL, struct akl_io_device);
    dev->iod_type = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos = 0;
    dev->iod_line_count = 1;
    dev->iod_char_count = 0;
    dev->iod_name = name;
    dev->iod_buffer = NULL;
    dev->iod_buffer_size = 0;
    akl_vector_init(&dev->iod_tokens, 30, sizeof(akl_token_t));
    return dev;
}

/* ~~~===### Free functions ###===~~~ */
/* Only used when every object is free()'d */
#if 0
void akl_free_list_entry(struct akl_state *in, struct akl_list_entry *ent)
{
    if (ent == NULL)
        return;

    akl_free_value(in,);
}

void akl_free_value(struct akl_state *s, struct akl_value *val)
{
    struct akl_userdata *data;
    struct akl_utype *utype;
    akl_destructor_t destroy;
    if (val == NULL)
        return;

    if (val->va_value.atom != NULL) {
        switch (val->va_type) {
            case TYPE_LIST:
            akl_free_list(val->va_value.list);
            break;

            case TYPE_ATOM:
            akl_free_atom(val->va_value.atom);
            break;

            case TYPE_STRING:
            /* TODO */
            break;

            case TYPE_USERDATA:
            data = akl_get_userdata_value(val);
            if (in) {
                utype = akl_vector_at(&s->ai_utypes, data->ud_id);
                if (utype) {
                    destroy = utype->ut_de_fun;
                /* Call the proper destructor function */
                /* The 'in->ai_utypes[data->ud_id]->ut_de_fun(in, data->ud_private);' */
                /* could be do this job, but then we could not protect  */
                /* ourselves from the NULL pointer dereference. */
                    if (destroy)
                        destroy(in, data->ud_private);
                } /* else: ERROR */
            } else {
                assert(in);
            }
            break;


            default:
            /* On NIL_* and TRUE_* values we don't need
              to decrease the reference count. */
            return;
        }
    }
    if (val->va_lex_info)
        AKL_FREE(val->va_lex_info);
    AKL_FREE(val);
}
void akl_free_atom(struct akl_state *in, struct akl_atom *atom)
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
#endif
