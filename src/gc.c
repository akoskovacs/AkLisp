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
#define BIT_INDEX(ptr, ind) ((ptr)[(ind)/BITS_IN_UINT])

#define SET_BIT(V, N) ((V) |= (1 << (N)))
#define CLEAR_BIT(V, N) ((V) &= ~(1 << (N)))
#define TEST_BIT(V, N) ((V) & (1 << (N)))
#define IS_BIT_SET(V, N) TEST_BIT(V, N)
#define IS_BIT_NOT_SET(V, N) (!TEST_BIT(V, N))

struct akl_mem_callbacks akl_mem_std_callbacks = {
    .mc_malloc_fn  = malloc,
    .mc_calloc_fn  = calloc,
    .mc_free_fn    = free,
    .mc_realloc_fn = realloc,
    .mc_nomem_fn   = akl_def_nomem_handler
};

void *akl_alloc(struct akl_state *s, size_t size)
{
    void *ptr;
    AKL_ASSERT(s, NULL);

    ptr = s->ai_mem_fn->mc_malloc_fn(size);
    s->ai_gc_malloc_size += size;
    if (ptr == NULL) {
        switch (s->ai_mem_fn->mc_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_alloc(s, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    }
    return ptr;
}

void *akl_calloc(struct akl_state *s, size_t nmemb, size_t size)
{
    void *ptr = NULL;
    AKL_ASSERT(s, NULL);

    ptr = s->ai_mem_fn->mc_calloc_fn(nmemb, size);
    s->ai_gc_malloc_size += size * nmemb;
    if (ptr == NULL) {
        switch (s->ai_mem_fn->mc_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_calloc(s, nmemb, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    }
    return ptr;
}

void *akl_realloc(struct akl_state *s, void *ptr, size_t size)
{
    void *p;
    AKL_ASSERT(s, NULL);

    p = s->ai_mem_fn->mc_realloc_fn(ptr, size);
    s->ai_gc_malloc_size += size;
    if (p == NULL) {
        switch (s->ai_mem_fn->mc_nomem_fn(s)) {
            case AKL_NM_TRYAGAIN:
            return akl_realloc(s, ptr, size);

            case AKL_NM_TERMINATE:
            exit(1); // FALLTHROUGH

            case AKL_NM_RETNULL:
            return NULL;
        }
    }
    return p;
}

void akl_free(struct akl_state *s, void *ptr, size_t size)
{
    if (s) {
        s->ai_mem_fn->mc_free_fn(ptr);
        s->ai_gc_malloc_size -= size;
    }
}

void akl_set_mem_callbacks(struct akl_state *s, const struct akl_mem_callbacks *cbs)
{
    AKL_ASSERT(s && cbs
    /* Every field must be filled properly */
        && cbs->mc_malloc_fn  && cbs->mc_calloc_fn
        && cbs->mc_realloc_fn && cbs->mc_free_fn
        && cbs->mc_nomem_fn, AKL_NOTHING);

     s->ai_mem_fn = cbs;
}

akl_gc_type_t akl_gc_register_type(struct akl_state *s, akl_gc_marker_t marker, size_t objsize)
{
    assert(s);
    struct akl_gc_type *t = (struct akl_gc_type *)akl_vector_reserve(&s->ai_gc_types);
    t->gt_marker_fn  = marker;
    t->gt_pool_count = 0;
    t->gt_pool_last  = NULL;
    t->gt_pool_head  = NULL;
    t->gt_type_id    = s->ai_gc_types.av_count-1;
    t->gt_type_size  = objsize;
    return t->gt_type_id;
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

static void akl_gc_mark_value(struct akl_state *s, void *obj, bool_t m)
{
    assert(obj);
    struct akl_value *v = (struct akl_value *)obj;
    if (v == &NIL_VALUE || v == &TRUE_VALUE)
        return;

    switch (v->va_type) {
        case AKL_VT_SYMBOL:
        /* Symbols are not GC'd */
        break;

        case AKL_VT_VARIABLE:
        /* Variables are also global */
        break;

        case AKL_VT_LIST:
        if (v->va_value.list)
            AKL_GC_SET_MARK(v->va_value.list, m);
        break;

        default:
        break;
    }
    AKL_GC_SET_MARK(v, m);
}

static void akl_gc_mark_list_entry(struct akl_state *s, void *obj, bool_t m)
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

static void
akl_gc_mark_variable(struct akl_state *s, void *obj, bool_t m)
{
    struct akl_variable *var = (struct akl_variable *)obj;
    AKL_GC_SET_MARK(var, m);
    if (var->vr_value) {
        akl_gc_mark_value(s, var->vr_value, m);
    }
}

static void
akl_gc_mark_function(struct akl_state *s, void *obj, bool_t m)
{
}

static void
akl_gc_mark_udata(struct akl_state *s, void *obj, bool_t m)
{
}

/* NOTE: Only call with value lists! */
static void akl_gc_mark_list(struct akl_state *s, void *obj, bool_t m)
{
    struct akl_list *list = (struct akl_list *)obj;
    struct akl_list_entry *ent;
    AKL_LIST_FOREACH(ent, list) {
        if (ent)
            akl_gc_mark_object(s, ent, m);
    }
    AKL_GC_SET_MARK(list, m);
}

void akl_gc_mark(struct akl_state *s)
{
    /* Walk around the IR and the variables */
}

void akl_gc_mark_all(struct akl_state *s)
{
    struct akl_atom *atom;
    if (!AKL_IS_FEATURE_ON(s, AKL_CFG_USE_GC)) 
        return;
#if 0
    RB_FOREACH(atom, ATOM_TREE, &s->ai_atom_head) {
        akl_gc_mark_atom(atom, TRUE);
    }
#endif

}

void akl_gc_unmark_all(struct akl_state *s)
{
    if (!AKL_IS_FEATURE_ON(s, AKL_CFG_USE_GC)) 
        return;
}

#define ASSERT_INDEX(ind) \
    assert(ind < AKL_GC_POOL_SIZE*(sizeof(unsigned int)*8))

bool_t akl_gc_pool_in_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
//    int i = ind/BITS_IN_UINT;
    return IS_BIT_SET(BIT_INDEX(p->gp_freemap, ind), ind);
}

void akl_gc_pool_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
    p->gp_pool.av_count++;
    SET_BIT(BIT_INDEX(p->gp_freemap, ind), ind);
}

void akl_gc_pool_clear_use(struct akl_gc_pool *p, unsigned int ind)
{
    assert(p);
    ASSERT_INDEX(ind);
    p->gp_pool.av_count--;
    CLEAR_BIT(BIT_INDEX(p->gp_freemap, ind), ind);
}

bool_t akl_gc_pool_have_free(struct akl_gc_pool *p)
{
    int i;
    assert(p);
    for (i = 0; i < AKL_GC_POOL_SIZE/BITS_IN_UINT; i++) {
        if (p->gp_freemap[i] != UINT_MAX) {
            return TRUE;
        }
    }
    return FALSE;
}

int akl_gc_pool_find_free(struct akl_gc_pool *p)
{
    int i, j;
    assert(p);
    /* Is the next element is usable? */
    int cnt = akl_vector_count(&p->gp_pool);
    if (cnt < AKL_GC_POOL_SIZE && !akl_gc_pool_in_use(p, cnt))
        return cnt;

    /* Nope... */
    for (i = 0; i < AKL_GC_POOL_SIZE/BITS_IN_UINT; i++) {
        if (p->gp_freemap[i] != UINT_MAX) {
            for (j = 0; j < BITS_IN_UINT; j++) {
                if (IS_BIT_NOT_SET(p->gp_freemap[i], j))
                    return j;
            }
        }
    }
    return -1;
}

static bool_t 
akl_gc_type_have_free(struct akl_state *s, struct akl_gc_type *t, struct akl_gc_pool **pool, unsigned int *ind)
{
    struct akl_gc_pool *p = t->gt_pool_head;
    int i;
    /* The last pool should have some free room (small optimization) */
    if (p == NULL) {
        *pool = akl_gc_pool_create(s, t);
        *ind = 0;
        return TRUE;
    }

    if (akl_gc_pool_have_free(t->gt_pool_last)) {
        *ind = akl_gc_pool_find_free(t->gt_pool_last);
        *pool = t->gt_pool_last;
        return TRUE;
    }

    /* Ok. We have to check the others too :-( */
    while (p && p != t->gt_pool_last) { /* The last was already checked... */
        /* TODO: Rework this */
        if (akl_gc_pool_have_free(p)) {
            i = akl_gc_pool_find_free(p);
            *pool = p;
            if (i == -1) {
                continue;
            }
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
    akl_vector_destroy(s, &p->gp_pool);
    akl_free(s, p->gp_freemap, AKL_GC_POOL_SIZE/BITS_IN_UINT);
    AKL_FREE(s, p);
}

bool_t akl_gc_type_tryfree(struct akl_state *s, struct akl_gc_type *t)
{
    assert(s && t);
    struct akl_gc_pool *prev = NULL;
    struct akl_gc_pool *next;
    /* Must start from the second pool. */
    struct akl_gc_pool *p = t->gt_pool_head;
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
    struct akl_gc_generic_object *go;
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
    akl_gc_sweep_pool(s, p->gp_next, marker);
}

void akl_gc_sweep(struct akl_state *s)
{
    int i;
    struct akl_gc_type *t;
    if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_GC)) {
        for (i = 0; i < akl_vector_count(&s->ai_gc_types); i++) {
            t = akl_gc_get_type(s, i);
            akl_gc_sweep_pool(s, t->gt_pool_head, t->gt_marker_fn);
        }
    }
}

void akl_gc_enable(struct akl_state *s)
{
    if (s) {
        AKL_SET_FEATURE(s, AKL_CFG_USE_GC);
    }
}

void akl_gc_disable(struct akl_state *s)
{
    if (s) {
        AKL_UNSET_FEATURE(s, AKL_CFG_USE_GC);
    }
}


struct akl_gc_type *akl_gc_get_type(struct akl_state *s, akl_gc_type_t type)
{
    assert(s);
    return (struct akl_gc_type *)akl_vector_at(&s->ai_gc_types, type);
}

struct akl_gc_pool *akl_gc_pool_create(struct akl_state *s, struct akl_gc_type *type)
{
    struct akl_gc_pool *pool = AKL_MALLOC(s, struct akl_gc_pool);
    pool->gp_next = NULL;
    akl_init_vector(s, &pool->gp_pool, AKL_GC_POOL_SIZE, type->gt_type_size);
    memset(pool->gp_freemap, 0, AKL_GC_POOL_SIZE/BITS_IN_UINT);

    if (type->gt_pool_last)
        type->gt_pool_last->gp_next = pool;

    if (type->gt_pool_head == NULL)
        type->gt_pool_head = pool;

    type->gt_pool_last = pool;
    type->gt_pool_count++;
    return pool;
}

// TODO: Implement akl_gc_mark_function and akl_gc_mark_udata
const akl_gc_marker_t base_type_markers[] = {
    akl_gc_mark_value, akl_gc_mark_variable, akl_gc_mark_list, akl_gc_mark_list_entry
  , akl_gc_mark_function, akl_gc_mark_udata
};

const size_t base_type_sizes[] = {
    sizeof(struct akl_value), sizeof(struct akl_variable), sizeof(struct akl_list)
  , sizeof(struct akl_list_entry), sizeof(struct akl_function)
  , sizeof(struct akl_userdata)
};

void akl_gc_init(struct akl_state *s)
{
    int i;
    s->ai_gc_last_was_mark = FALSE;
    s->ai_gc_malloc_size = 0;

    akl_gc_disable(s);
    akl_init_vector(s, &s->ai_gc_types, AKL_GC_NR_BASE_TYPES, sizeof(struct akl_gc_type));
    for (i = 0; i < AKL_GC_NR_BASE_TYPES; i++) {
        akl_gc_register_type(s, base_type_markers[i], base_type_sizes[i]);
    }
}
/**
 * @brief Request memory from the GC
 * @param s An instance of the interpreter (cannot be NULL)
 * @param type Type of the GC object
 * @see AKL_GC_OBJECT_TYPE
 * If the claim is not met, try to collect some memory or
 * create a new GC pool.
*/
void *akl_gc_malloc(struct akl_state *s, akl_gc_type_t tid)
{
    assert(s && tid < akl_vector_count(&s->ai_gc_types));
    struct akl_gc_type *t = akl_gc_get_type(s, tid);
    struct akl_gc_pool *p;
    unsigned int ind;
    if (akl_gc_type_have_free(s, t, &p, &ind)) {
        /* This was too easy... */
        akl_gc_pool_use(p, ind);
        return akl_vector_at(&p->gp_pool, ind);
    } else {
        if (!s->ai_gc_last_was_mark && AKL_IS_FEATURE_ON(s, AKL_CFG_USE_GC)) {
            akl_gc_mark(s);
            akl_gc_sweep(s);
            s->ai_gc_last_was_mark = TRUE;
            return akl_gc_malloc(s, tid);
        } else {
            /* We cannot collect enough memory :-( => we have to to some
             * allocation. To do this we must also need to create the
             * container pool (just for that type). */
            p = akl_gc_pool_create(s, t);
            s->ai_gc_last_was_mark = FALSE;
            akl_gc_pool_use(p, 0);
            return akl_vector_at(&p->gp_pool, 0);
        }
    }
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
