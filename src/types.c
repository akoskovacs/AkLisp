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

/* ~~~===### VECTOR ###===~~~ */
struct akl_vector *
akl_vector_new(struct akl_state *s, unsigned int nmemb, unsigned int size)
{
    struct akl_vector *vec = AKL_MALLOC(s, struct akl_vector);

    akl_vector_init(vec, nmemb, size);
    return vec;
}

void akl_vector_init(struct akl_vector *vec, unsigned int nmemb, unsigned int size)
{
    assert(vec);
    if (!nmemb)
        nmemb = AKL_VECTOR_DEFSIZE;

    if (!size)
        size = sizeof(void *);

    vec->av_vector = CALLOC_FUNCTION(nmemb, size);
    vec->av_size = nmemb;
    vec->av_count = 0;
    vec->av_msize = size;
}

bool_t akl_vector_is_empty(struct akl_vector *vec)
{
    return (!vec || !vec->av_vector || vec->av_count == 0);
}

/**
 @brief Remove and return with the last element of a vector
*/
void *akl_vector_pop(struct akl_vector *vec)
{
    assert(vec);
    if (vec->av_count == 0)
        return NULL;
    return  akl_vector_at(vec, --vec->av_count);
}

/* Need to grow for the next push? */
bool_t akl_vector_is_grow_need(struct akl_vector *vec)
{
   if (vec && vec->av_count+1 >= vec->av_size)
       return TRUE;

   return FALSE;
}

void *akl_vector_reserve(struct akl_vector *vec)
{
    if (akl_vector_is_grow_need(vec))
        akl_vector_grow(vec);

    return akl_vector_at(vec, vec->av_count++);
}

void *akl_vector_reserve_more(struct akl_vector *vec, int cnt)
{
    assert(cnt >= 0);
    void *start = akl_vector_reserve(vec);
    cnt--;
    while (cnt--) {
        /* I don't care. The space must be contignous */
        (void)akl_vector_reserve(vec);
    }
    return start;
}

unsigned int
akl_vector_push(struct akl_vector *vec, void *data)
{
    void *ptr;
    assert(vec);
    ptr = akl_vector_reserve(vec);
    memcpy(ptr, data, vec->av_msize);
    return vec->av_count-1;
}

void akl_vector_push_vec(struct akl_vector *vec, struct akl_vector *v)
{
    void *ptr;
    if (vec->av_msize != v->av_msize)
        return;

    if (vec->av_count + v->av_count + 1 < vec->av_size) {
        akl_vector_grow(vec);
        akl_vector_push_vec(vec, v);
        return;
    }

    ptr = akl_vector_at(vec, vec->av_count+1);
    memcpy(ptr, v->av_vector, v->av_count);
    vec->av_count += v->av_count;
}

/** @brief Find a specific element in a vector
  * @param vec The vector
  * @param cmp_fn Comparison function
  * @param arg The argument second argument of the comparison function
  * @param ind Pointer to an unsigned int, which will store the index
*/
void *akl_vector_find(struct akl_vector *vec
      , akl_cmp_fn_t cmp_fn, void *arg, unsigned int *ind)
{
    unsigned int i;
    void *at;
    assert(vec);
    for (i = 0; i < vec->av_size; i++) {
        at = akl_vector_at(vec, i);
        if (cmp_fn(at, arg)) {
            if (ind != NULL)
                *ind = i;
            return at;
        }
    }
    return NULL;
}

void akl_vector_grow(struct akl_vector *vec)
{
    assert(vec);
    int osize = vec->av_size;
    struct akl_byte_t *bmap;
    if (vec->av_size > 30)
        vec->av_size = vec->av_size + (vec->av_size/2);
    else
        vec->av_size = vec->av_size * 2;
    vec->av_vector = REALLOC_FUNCTION(vec->av_vector, vec->av_size*vec->av_msize);
}

/* The user should zero out the removed memory */
void *akl_vector_remove(struct akl_vector *vec, unsigned int ind)
{
    /* TODO */
}

inline void *
akl_vector_at(struct akl_vector *vec, unsigned int ind)
{
    assert(vec);
    if (ind < vec->av_count) {
        return vec->av_vector + (ind * vec->av_msize);
    }
    return NULL;
}

inline void *
akl_vector_last(struct akl_vector *vec)
{
    return akl_vector_at(vec, vec->av_count);
}

inline void *
akl_vector_first(struct akl_vector *vec)
{
    return akl_vector_at(vec, 0);
}

inline unsigned int
akl_vector_size(struct akl_vector *vec)
{
    assert(vec);
    return vec->av_size;
}

inline unsigned int
akl_vector_count(struct akl_vector *vec)
{
    assert(vec);
    return vec->av_count;
}

void akl_vector_destroy(struct akl_vector *vec)
{
    FREE_FUNCTION(vec->av_vector);
}

void akl_vector_free(struct akl_vector *vec)
{
    akl_vector_destroy(vec);
    FREE_FUNCTION(vec);
}

/* ~~~===### Stack handling ###===~~~ */
void akl_stack_init(struct akl_state *s)
{
    assert(s);
    if (s->ai_stack.av_vector == NULL) {
        akl_vector_init(s->ai_stack, 50, sizeof(struct akl_value *));
    }
}
/* Attention: The stack contains pointer to value pointers */
void akl_stack_push(struct akl_state *s, struct akl_value *value)
{
    assert(s);
    akl_vector_push(s->ai_stack, &value);
}

struct akl_value *akl_stack_pop(struct akl_state *s)
{
    assert(s);
    return *akl_vector_pop(s);
}

/* These functions do not check the type of the stack top */
double akl_stack_pop_number(struct akl_state *s)
{
    struct akl_value *v = (s) ? akl_stack_pop(s) : NULL;
    return (v) ? v->va_value.number : NULL;
}

char *akl_stack_pop_string(struct akl_state *s)
{
    struct akl_value *v = (s) ? akl_stack_pop(s) : NULL;
    return (v) ? v->va_value.string : NULL;
}

struct akl_list *akl_stack_pop_list(struct akl_state *s)
{
    struct akl_value *v = (s) ? akl_stack_pop(s) : NULL;
    return (v) ? v->va_value.list : NULL;
}

enum AKL_VALUE_TYPE akl_stack_top_type(struct akl_state *s)
{
    struct akl_value *v = (s) ? akl_stack_pop(s) : NULL;
    return (v) ? v->va_type : TYPE_NIL;
}

/* ~~~===### Some getter and conversion functions ###===~~~ */

char *akl_get_atom_name_value(struct akl_value *val)
{
    struct akl_atom *atom = AKL_GET_ATOM_VALUE(val);
    return (atom != NULL) ? atom->at_name : NULL;
}

/* NOTE: These functions give back a NULL pointer, if the conversion
  cannot be completed */
struct akl_value *akl_to_number(struct akl_state *in, struct akl_value *v)
{
    char *str = NULL;
    if (v) {
        switch (v->va_type) {
            case TYPE_STRING:
            str = AKL_GET_STRING_VALUE(v);
            break;

            case TYPE_ATOM:
            if (v->va_value.atom != NULL)
                str = v->va_value.atom->at_name;
            break;

            case TYPE_NIL:
            str = "0";
            break;

            case TYPE_TRUE:
            str = "1";
            break;

            case TYPE_NUMBER:
            return v;

            default:
            break;
        }
        if (str)
            return akl_new_number_value(in, atof(str));
    }
    return NULL;
}

char *akl_num_to_str(struct akl_state *in, double number)
{
    int strsize = 30;
    char *str = (char *)akl_malloc(in, strsize);
    while (snprintf(str, strsize, "%g", number) >= strsize) {
        strsize += strsize/2;
        str = realloc(str, strsize);
    }
    return str;
}

struct akl_value *akl_to_string(struct akl_state *in, struct akl_value *v)
{
    const char *str;
    if (v) {
        switch (v->va_type) {
            case TYPE_NUMBER:
            str = akl_num_to_str(in, AKL_GET_NUMBER_VALUE(v));
            break;

            case TYPE_ATOM:
            if (v->va_value.atom != NULL)
                str = v->va_value.atom->at_name;
            break;

            case TYPE_NIL:
            str = "NIL";
            break;

            case TYPE_TRUE:
            str = "T";
            break;

            case TYPE_STRING:
            return v;

            default:
            break;
        }
        if (str)
            return akl_new_string_value(in, strdup(str));
    }
    return NULL;
}

struct akl_value *akl_to_symbol(struct akl_state *in, struct akl_value *v)
{
    struct akl_value *sym;
    char *name = NULL;
    if (v) {
        switch (v->va_type) {
            case TYPE_NUMBER:
            name = akl_num_to_str(in, AKL_GET_NUMBER_VALUE(v));
            break;

            case TYPE_STRING:
            /* TODO: Eliminate the strup()s */
            name = strdup(AKL_GET_STRING_VALUE(v));
            break;

            case TYPE_NIL:
            name = strdup("NIL");
            break;

            case TYPE_TRUE:
            name = strdup("T");
            break;

            case TYPE_ATOM:
            if (v->va_value.atom != NULL)
                name = strdup(v->va_value.atom->at_name);
            break;

            default:
            break;
        }
        if (name) {
            sym = akl_new_atom_value(in, name);
            sym->is_quoted = TRUE;
            return sym;
        }
    }
    return NULL;
}

/* ~~~===### UTYPE ###===~~~ */

struct akl_userdata *akl_get_userdata_value(struct akl_value *value)
{
    struct akl_userdata *data;
    if (AKL_CHECK_TYPE(value, TYPE_USERDATA)) {
       data = value->va_value.udata;
       return data;
    }
    return NULL;
}

void *akl_get_udata_value(struct akl_value *value)
{
    struct akl_userdata *data;
    data = akl_get_userdata_value(value);
    if (data)
        return data->ud_private;
    return NULL;
}

bool_t akl_check_user_type(struct akl_value *v, akl_utype_t type)
{
    struct akl_userdata *d = akl_get_userdata_value(v);
    if (d && d->ud_id == type)
        return TRUE;
    return FALSE;
}

struct akl_module *akl_get_module_descriptor(struct akl_state *in, struct akl_value *v)
{
    struct akl_userdata *data;
    if (in && AKL_CHECK_TYPE(v, TYPE_USERDATA)) {
       data = akl_get_userdata_value(v);
       return akl_vector_at(&in->ai_modules, data->ud_id);
    }
    return NULL;
}

unsigned int akl_get_utype_value(struct akl_value *value)
{
    struct akl_userdata *data;
    data = akl_get_userdata_value(value);
    if (data)
        return data->ud_id;
    return (unsigned int)-1;
}

unsigned int
akl_register_type(struct akl_state *s, const char *name, akl_gc_destructor_t de_fun)
{
    struct akl_utype *type = AKL_MALLOC(s, struct akl_utype);
    type->ut_name = name;
    type->ut_de_fun = de_fun;

    return akl_vector_add(&s->ai_utypes, type);
}

void akl_deregister_type(struct akl_state *s, unsigned int type)
{
    AKL_FREE(akl_vector_remove(&s->ai_utypes, type));
}


static bool_t utype_finder_name(void *t, void *name)
{
    struct akl_utype *type = (struct akl_utype *)t;
    if (t && name
        && (strcmp(type->ut_name, (const char *)name) == 0))
        return TRUE;

    return FALSE;
}

int akl_get_typeid(struct akl_state *in, const char *tname)
{
    struct akl_utype *utype = NULL;
    unsigned int ind;
    utype = akl_vector_find(&in->ai_utypes, utype_finder_name
                                        , (void *)tname, &ind);
    if (utype)
        return ind;

    return -1;
}
