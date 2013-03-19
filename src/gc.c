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
#include <limits.h>
#include "aklisp.h"

#define BITS_IN_UINT (sizeof(unsigned int)*8)
#define INT_INDEX(iary) iary[AKL_GC_POOL_SIZE/BITS_IN_UINT-1]
#define BIT_INDEX_MASK(bi) (1 << ((bi)%BITS_IN_UINT-1))
#define BIT_IS_SET(byte, bi) (byte) & BIT_INDEX_MASK(bi)
#define BIT_SET(byte, bi) (byte) |= BIT_INDEX_MASK(bi)
#define BIT_CLEAR(byte, bi) (byte) ^= BIT_INDEX_MASK(bi)

static struct akl_gc_object vobj = { 
    AKL_GC_VALUE
   , FALSE, FALSE
   , TRUE 
};

static struct akl_gc_object aobj = { 
     AKL_GC_ATOM
   , FALSE, FALSE
   , FALSE 
};

#if _GNUC_
struct akl_value TRUE_VALUE = {
	.gc_obj = vobj,
    .va_type = TYPE_TRUE,
    .va_value.number = 1,
    .is_quoted = TRUE,
    .is_nil = FALSE,
    .va_lex_info = NULL,
};

struct akl_value NIL_VALUE = {
	.gc_obj = vobj,
    .va_type = TYPE_NIL,
    .va_value.number = 0,
    .is_quoted = TRUE,
    .is_nil = TRUE,
    .va_lex_info = NULL,
};
#else
struct akl_value TRUE_VALUE = { 
	vobj
  , NULL, TYPE_TRUE, (double)0
  , FALSE, FALSE
};

struct akl_value FALSE_VALUE = { 
	vobj
  , NULL, TYPE_TRUE, (double)0
  , FALSE, TRUE
};
#endif // _GNU_

static akl_nomem_action_t
akl_def_nomem_handler(struct akl_state *s)
{
    if (akl_gc_tryfree(s))
        return AKL_NM_TRYAGAIN;
    else
        fprintf(stderr, "ERROR! No memory left!\n");

    return AKL_NM_TERMINATE;
}

void *akl_alloc(struct akl_state *s, size_t size)
{
    void *ptr;
    assert(s && s->ai_malloc_fn);

    ptr = s->ai_malloc_fn(size);
    s->ai_gc_malloc_size += size;
    if (ptr == NULL) {
        if (!s->ai_nomem_fn)
            s->ai_nomem_fn = akl_def_nomem_handler;

        switch (s->ai_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_alloc(s, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    } else {
        return ptr;
    }
}

void *akl_calloc(struct akl_state *s, size_t nmemb, size_t size)
{
    void *ptr;
    assert(s && s->ai_calloc_fn);

    ptr = s->ai_calloc_fn(nmemb, size);
    s->ai_gc_malloc_size += size;
    if (ptr == NULL) {
        if (!s->ai_nomem_fn)
            s->ai_nomem_fn = akl_def_nomem_handler;

        switch (s->ai_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_calloc(s, nmemb, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    } else {
        return ptr;
    }
}

void *akl_realloc(struct akl_state *s, void *ptr, size_t size)
{
    void *ptr;
    assert(s && s->ai_realloc_fn);

    ptr = s->ai_realloc_fn(ptr, size);
    s->ai_gc_malloc_size += size;
    if (ptr == NULL) {
        if (!s->ai_nomem_fn)
            s->ai_nomem_fn = akl_def_nomem_handler;

        switch (s->ai_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_realloc(s, ptr, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    } else {
        return ptr;
    }
}

void akl_free(struct akl_state *s, void *ptr, size_t size)
{
    if (s && s->ai_free_fn) {
        s->ai_free_fn(ptr);
        s->ai_gc_malloc_size -= size;
    } else {
        free(ptr);
    }
}

akl_gc_type_t akl_gc_register_type(struct akl_state *s, akl_gc_marker_t marker, size_t objsize)
{
    assert(s);
    struct akl_gc_type *t = (struct akl_gc_type *)akl_vector_reserve(&s->ai_gc_types);
    t->gt_marker_fn = marker;
    t->gt_type_id = akl_vector_count(&s->ai_gc_types)-1;
    t->gt_pool_count = 1;
    akl_gc_init_pool(s, &t->gt_pools, t->gt_type_id);
    t->gt_pool_last = &t->gt_pools;
    return t->gt_type_id;
}

struct akl_state *
akl_new_file_interpreter(const char *file_name, FILE *fp, void *(*alloc)(size_t))
{
    struct akl_state *in = akl_new_state(alloc);
    in->ai_device = akl_new_file_device(file_name, fp, alloc);
    return in;
}

struct akl_state *
akl_new_string_interpreter(const char *name, const char *str, void *(*alloc)(size_t))
{
    struct akl_state *s = akl_new_state(alloc);
    s->ai_device = akl_new_string_device(name, str, alloc);
    return s;
}

struct akl_state *
akl_reset_string_interpreter(struct akl_state *in, const char *name, const char *str, void (*alloc)(size_t))
{
   if (in == NULL) {
       return akl_new_string_interpreter(name, str, alloc);
   } else if (in->ai_device == NULL) {
       in->ai_device = akl_new_string_device(name, str, alloc);
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
   if (in && in->ai_device == NULL) {
       in->ai_device = akl_new_file_device(name, fp, in->ai_malloc_fn);
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

void akl_gc_mark_object(struct akl_state *s, void *obj, bool_t m)
{
    struct akl_gc_generic_object *gobj = (struct akl_gc_generic_object *)obj;
    struct akl_gc_type *t = akl_gc_get_type(s, AKL_GC_TYPE_ID(gobj));
    if (t && t->gt_marker_fn) {
        t->gt_marker_fn(s, obj, m);
    }
}

void akl_gc_mark_value(struct akl_state *s, void *obj, bool_t m)
{
    assert(obj);
    struct akl_value *v = (struct akl_value *)obj;
    if (v == &NIL_VALUE || v == &TRUE_VALUE)
        return;

    switch (v->va_type) {
        case TYPE_ATOM:
        if (v->va_value.atom)
            AKL_GC_SET_MARK(v->va_value.atom, m);
        break;

        case TYPE_LIST:
        if (v->va_value.list)
            AKL_GC_SET_MARK(v->va_value.list, m);
        break;

        default:
        break;
    }
    AKL_GC_SET_MARK(v, m);
}

void akl_gc_mark_atom(struct akl_state *s, void *obj, bool_t m)
{
    assert(obj);
    struct akl_atom *atom = (struct akl_atom *)obj;
    AKL_GC_SET_MARK(atom, m);
    if (atom->at_value)
        akl_gc_mark_object(s, atom->at_value, m);
}

void akl_gc_mark_list_entry(struct akl_state *s, void *obj, bool_t m)
{
    assert(obj);
    struct akl_value *v;
    struct akl_list_entry *le = (struct akl_list_entry *)obj;
    AKL_GC_SET_MARK(le, m);
    if (le->gc_obj.gc_le_is_obj) {
        v = (struct akl_value *)le->le_data;
        if (v)
            akl_gc_mark_object(s, v, m);
    }
}

/* NOTE: Only call with value lists! */
void akl_gc_mark_list(struct akl_state *s, void *obj, bool_t m)
{
    struct akl_list *list = (struct akl_list *)obj;
    struct akl_list_entry *ent;
    AKL_LIST_FOREACH(ent, list) {
        if (ent)
            akl_gc_mark_object(s, ent, m);
    }
    AKL_GC_SET_MARK(list, m);
}

void akl_gc_mark_all(struct akl_state *s)
{
    struct akl_atom *atom;
    if (!s->ai_gc_is_enabled)
        return;

    RB_FOREACH(atom, ATOM_TREE, &s->ai_atom_head) {
        akl_gc_mark_atom(atom, TRUE);
    }

    /* Walk around the IR and the variables */
}

void akl_gc_unmark_all(struct akl_state *s)
{
    if (!s->ai_gc_is_enabled)
        return;
}

#define ASSERT_INDEX(ind) \
    assert(ind < AKL_GC_POOL_SIZE*(sizeof(unsigned int)*8))

bool_t akl_gc_pool_in_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
    BIT_IS_SET(INT_INDEX(p->gp_freemap), ind);
}

void akl_gc_pool_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
    BIT_SET(INT_INDEX(p->gp_freemap), ind);
}

void akl_gc_pool_clear_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
    BIT_CLEAR(INT_INDEX(p->gp_freemap), ind);
}


bool_t akl_gc_pool_have_free(struct akl_gc_pool *p)
{
    int i;
    assert(p);
    for (i = 0; i < AKL_GC_POOL_SIZE/BITS_IN_UINT; i++) {
        if (p->gp_freemap[i] != UINT_MAX)
            return TRUE;
    }
    return FALSE;
}

int akl_gc_pool_find_free(struct akl_gc_pool *p)
{
    int i, j;
    assert(p);
    /* Is the next element is usable? */
    int cnt = akl_vector_count(&p->gp_pool);
    if (cnt < AKL_GC_POOL_SIZE && akl_gc_pool_in_use(p, cnt))
        return cnt;
    
    /* Nope... */
    for (i = 0; i < AKL_GC_POOL_SIZE/BITS_IN_UINT; i++) {
        if (p->gp_freemap[i] != UINT_MAX) {
            for (j = 0; j < BITS_IN_UINT; j++) {
                if (!BIT_IS_SET(p->gp_freemap[i], j))
                    return j;
            }
        }
    }
    return -1;
}

bool_t akl_gc_type_have_free(struct akl_gc_type *t, struct akl_gc_pool **pool, unsigned int *ind)
{
    struct akl_gc_pool *p = &t->gt_pools;
    /* The last pool should have some free room (small optimization) */
    if (akl_gc_pool_have_free(t->gt_pool_last)) {
        *ind = akl_gc_pool_find_free(t->gt_pool_last);
        *pool = t->gt_pool_last;
        return TRUE;
    }

    /* Ok. We have to check the others too :-( */
    while (p && p != t->gt_pool_last) { /* The last was already checked... */
        if (akl_gc_pool_have_free(p)) {
            *ind = akl_gc_pool_find_free(p);
            *pool = p;
            return TRUE;
        }
        p = p->gp_next;
    }
    return FALSE;
}

bool_t akl_gc_pool_is_empty(struct akl_gc_pool *p)
{
    assert(p);
    int i, s = 0;
    for (i = 0; i < AKL_GC_POOL_SIZE/BITS_IN_UINT; i++) {
        s += p->gp_freemap[i];
    }
    return s == 0;
} 

void akl_gc_pool_free(struct akl_state *s, struct akl_gc_pool *p)
{
    akl_vector_destroy(&p->gp_pool);
    akl_free(s, p->gp_freemap, AKL_GC_POOL_SIZE/BITS_IN_UINT);
    AKL_FREE(s, p);
}

bool_t akl_gc_type_tryfree(struct akl_state *s, struct akl_gc_type *t)
{
    assert(s && t);
    struct akl_gc_pool *prev = NULL;
    struct akl_gc_pool *next;
    /* Must start from the second pool. */
    struct akl_gc_pool *p = t->gt_pools.gp_next;
    bool_t succeed = FALSE;

    while (p) {
        next = p->gp_next;
        if (akl_gc_pool_is_empty(p)) {
            succeed = TRUE;
            if (prev)
                prev->gp_next = next;

            /* This was the last pool, update the 'last' pointer */
            if (next == NULL)
                t->gt_pool_last = prev;
            akl_gc_pool_free(s, p);
        } else {
            prev = p;
        }
        p = next;
    }
    return succeed;
}

bool_t akl_gc_tryfree(struct akl_state *s)
{
    assert(s);
    bool_t succeed = FALSE;
    int i; 
    for (i = 0; i < akl_vector_count(&s->ai_gc_types); i++) {
        if (akl_gc_type_tryfree(s, akl_gc_get_type(s, i)))
            succeed = TRUE;
    }
    return succeed;
}

void akl_gc_sweep_pool(struct akl_state *s, struct akl_gc_pool *p, akl_gc_marker_t marker)
{
    struct akl_vector *v;
    void *p;
    struct akl_gc_generic_object *go;
    akl_gc_destructor_t defun;
    int i;
    if (!p || !marker)
        return;

    v = &p->gp_pool;
    for (i = 0; i < AKL_GC_POOL_SIZE; i++) {
        go = (struct akl_gc_generic_object *)akl_vector_at(v, i);
        if (akl_gc_pool_in_use(p, i)) {
            if (AKL_GC_IS_MARKED(go)) {
                marker(s, go, FALSE);
            } else {
                akl_gc_pool_clear_use(p, i);
            }
        }
    }
    akl_gc_sweep_pool(p->gp_next);
}

void akl_gc_sweep(struct akl_state *s)
{
    int i;
    struct akl_gc_type *t;
    if (s->ai_gc_is_enabled) {
        for (i = 0; i < akl_vector_count(&s->ai_gc_types); i++) {
            t = akl_gc_get_type(s, i);
            akl_gc_sweep_pool(s, &t->gt_pools, t->gt_marker_fn);
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


struct akl_gc_type *akl_gc_get_type(struct akl_state *s, akl_gc_type_t type)
{
    assert(s);
    return (struct akl_gc_type *)akl_vector_at(&s->ai_gc_types, type);
}

static void akl_gc_init_pool(struct akl_state *s, struct akl_gc_pool *p
                      , akl_gc_type_t type, size_t osize)
{
    p->gp_next = NULL;
    // p->gp_type = type;
    akl_vector_init(s, &p->gp_pool, AKL_GC_POOL_SIZE, osize);
    memset(p->gp_freemap, 0, AKL_GC_POOL_SIZE/BITS_IN_UINT);
}

struct akl_gc_pool *akl_new_gc_pool(struct akl_state *s
                                    , enum AKL_GC_OBJECT_TYPE type)
{
    struct akl_gc_pool *pool;
    pool = AKL_MALLOC(s, struct akl_gc_pool);
    akl_gc_init_pool(s, pool, type);
    return pool;
}

struct akl_function *akl_new_function(struct akl_state *s)
{
    struct akl_function *func = (struct akl_function *)
                        akl_gc_malloc(s, AKL_GC_FUNCTION);
    //func->fn_argc = 0;
    //func->fn_body = NULL;
    return func;
}

// TODO: Implement akl_gc_mark_function and akl_gc_mark_udata
const akl_gc_marker_t base_type_markers[] = {
    akl_gc_mark_value, akl_gc_mark_atom, akl_gc_mark_list
  , akl_gc_mark_list_entry, akl_gc_mark_function, akl_gc_mark_udata 
};

const size_t base_type_sizes[] = {
    sizeof(struct akl_value), sizeof(struct akl_atom), sizeof(struct akl_list)
  , sizeof(struct akl_list_entry), sizeof(struct akl_function), sizeof(struct akl_userdata)
};

void akl_gc_init(struct akl_state *s)
{
    int i;
    akl_gc_disable(s);
    for (i = 0; i < AKL_GC_NR_BASE_TYPES; i++) {
        akl_gc_register_type(s, base_type_markers[i], base_type_sizes[i]);
    }
}

void akl_init_state(struct akl_state *s)
{
    RB_INIT(&s->ai_atom_head);
    s->ai_device = NULL;
    akl_init_list(&s->ai_modules);
    akl_vector_init(s, &s->ai_utypes, sizeof(struct akl_module *), 5);
    s->ai_program  = NULL;
    s->ai_errors   = NULL;

    s->ai_malloc_fn = malloc;
    s->ai_realloc_fn = realloc;
    s->ai_calloc_fn = calloc;
    s->ai_free_fn = free;
    s->ai_nomem_fn = akl_def_nomem_handler;

    akl_gc_init(s);
}

struct akl_state *akl_new_state(void *(*alloc)(size_t))
{
    struct akl_state *s;
    if (!alloc) {
         alloc = malloc;
    }
    s = (struct akl_state *)alloc(sizeof(struct akl_state));
    akl_init_state(s);
    s->ai_malloc_fn = alloc;
    return s;
}

/**
 * @brief Request memory from the GC
 * @param s An instance of the interpreter (cannot be NULL)
 * @param type Type of the GC object
 * @see AKL_GC_OBJECT_TYPE
 * If the claim is not met, try to collect some memory or
 * create a new GC pool.
*/
void *akl_gc_malloc(struct akl_state *s, akl_gc_type_t type)
{
    assert(s && type < akl_vector_count(&s->ai_gc_types));
    static bool_t last_was_mark = FALSE;
    struct akl_gc_type *t = akl_gc_get_type(s, type);
    struct akl_gc_pool *p;
    unsigned int ind;
    if (akl_gc_type_have_free(t, &p, &ind)) {
        /* This was too easy... */
        akl_gc_pool_use(p, ind);
        return akl_vector_at(&p->gp_pool, (unsigned int)ind);
    } else {
        if (!last_was_mark && s->ai_gc_is_enabled) {
            akl_gc_mark(s);
            akl_gc_sweep(s);
            last_was_mark = TRUE;
            return akl_gc_malloc(s, type);
        } else {
            p->gp_next = akl_new_gc_pool(s, type);
            t->gt_pool_last = p->gp_next;
            last_was_mark = FALSE;
            return akl_gc_malloc(s, type);
        }
    }
}

void akl_init_list(struct akl_list *list)
{
    AKL_GC_INIT_OBJ(list, AKL_GC_LIST);
    list->li_head   = NULL;
    list->li_last   = NULL;
    list->is_quoted = FALSE;
    list->is_nil    = FALSE;
    list->li_parent = NULL;
    list->li_elem_count  = 0;
}

struct akl_list *akl_new_list(struct akl_state *s)
{
    struct akl_list *list = (struct akl_list *)akl_gc_malloc(s, AKL_GC_LIST);
    akl_init_list(list);
    return list;
}

struct akl_atom *akl_new_atom(struct akl_state *s, char *name)
{
    struct akl_atom *atom = (struct akl_atom *)akl_gc_malloc(s, AKL_GC_ATOM);
    AKL_GC_INIT_OBJ(atom, AKL_GC_ATOM);
    assert(name);
    atom->at_value = NULL;
    atom->at_name  = name;
    atom->at_desc  = NULL;
    return atom;
}

struct akl_list_entry *akl_new_list_entry(struct akl_state *s)
{
    struct akl_list_entry *ent =
            (struct akl_list_entry *)akl_gc_malloc(s, AKL_GC_LIST_ENTRY);
    AKL_GC_INIT_OBJ(ent, AKL_GC_LIST_ENTRY);
    ent->le_data = NULL;
    ent->le_next = NULL;
    ent->le_prev = NULL;
    return ent;
}

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
    AKL_GC_INIT_OBJ(val, AKL_GC_VALUE);
    val->is_nil    = FALSE;
    val->is_quoted = FALSE;
    val->va_lex_info = NULL;
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

struct akl_value *akl_new_list_value(struct akl_state *s, struct akl_list *lh)
{
    struct akl_value *val = akl_new_value(s);
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
akl_new_file_device(const char *file_name, FILE *fp, void *(*alloc)(size_t))
{
    struct akl_io_device *dev;
    if (!alloc)
        alloc = malloc;

    dev = (struct akl_io_device *)alloc(sizeof(struct akl_io_device));
    dev->iod_type = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos = 0;
    dev->iod_line_count = 1;
    dev->iod_name = file_name;
    dev->iod_buffer = NULL;
    dev->iod_buffer_size = 0;
    return dev;
}

struct akl_io_device *
akl_new_string_device(const char *name, const char *str, void *(*alloc)(size_t))
{
    struct akl_io_device *dev;
    if (!alloc)
        alloc = malloc;

    dev = (struct akl_io_device *)alloc(sizeof(struct akl_io_device));
    dev->iod_type = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos = 0;
    dev->iod_line_count = 1;
    dev->iod_char_count = 0;
    dev->iod_name = name;
    dev->iod_buffer = NULL;
    dev->iod_buffer_size = 0;
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
