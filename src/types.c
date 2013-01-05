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
#include <limits.h>

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
    /* Only useful, when remove used */
    vec->av_free = NULL;
}

/**
 * @brief Create an initial free list (bitmap) for an existing vector
 * @param vec An instance of the vector
 * @return TRUE if it was the first call
 * It is also mark all used elements.
*/
bool_t akl_vector_init_free(struct akl_vector *vec)
{
    if (vec && !vec->av_free) {
        vec->av_free = (struct akl_byte_t *)
            CALLOC_FUNCTION((vec->av_size+1)/8, sizeof(akl_byte_t));

        /* Because this is the first time, when akl_vector_init_free() runs, 
         mark everything. */
        akl_vector_mark_all(vec);
        return TRUE;
    }
    return FALSE;
}

#define BIT_INDEX_MASK(bi) (1 << ((bi)%8-1))
#define BIT_IS_SET(byte, bi) (byte) & BIT_INDEX_MASK(bi)
#define BIT_SET(byte, bi) (byte) |= BIT_INDEX_MASK(bi)
#define BIT_CLEAR(byte, bi) (byte) ^= BIT_INDEX_MASK(bi) 

/**
 * @brief Mark a vector element 'used' in the free list
 * @param vec An existing vector
 * @param ind Index of the element
*/
void akl_vector_set_mark(struct akl_vector *vec, unsigned int ind)
{
    if (!vec || !vec->av_free || ind > vec->av_size)
        return;

    BIT_SET(vec->av_free[ind/8], ind);
}

/**
 * @brief Mark a vector element 'free' in the free list
 * @param vec An existing vector
 * @param ind Index of the element
*/
void akl_vector_clear_mark(struct akl_vector *vec, unsigned int ind)
{
    if (!vec || !vec->av_free || ind > vec->av_size)
        return;

    BIT_CLEAR(vec->av_free[ind/8], ind);
}

void akl_vector_mark_all(struct akl_vector *vec)
{
    int i;
    for (i = 0; i < vec->av_count; i++) {
        akl_vector_set_mark(vec, i);
    }
}

bool_t akl_vector_is_empty(struct akl_vector *vec)
{
    return (!vec || !vec->av_vector || vec->av_count == 0);
}

/** 
 * @brief Gives back \c TRUE if the element is not used at the given index
 * @param vec Instance of the vector
 * @param ind Index of the element
*/
bool_t akl_vector_is_empty_at(struct akl_vector *vec, unsigned int ind)
{
    assert(vec);
    return BIT_IS_SET(vec->av_free[ind/8], ind);
}

void *akl_vector_pop(struct akl_vector *vec)
{
    assert(vec);
    if (vec->av_count == 0)
        return NULL;
    akl_vector_clear_mark(vec, vec->av_count-1);
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

    /* Mark the current element used 
     and give back it's address */
    akl_vector_set_mark(vec, vec->av_count);
    return akl_vector_at(vec, vec->av_count++);
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

/** 
 * @brief Find a free index
 * @return With the index of a free element, otherwise -1
*/
int akl_vector_get_free(struct akl_vector *vec)
{
    int i, j;
    assert(vec);

    if (akl_vector_init_free(vec))
        return -1;

    for (i = 0; i < vec->av_size; i++) {
        /* Are there any zeroed bits? */
        if (vec->av_free[i] < UCHAR_MAX) {
            for (j = 0; i < 8; j++)
                if (BIT_IS_SET(vec->av_free[i], j))
                    return i*8+j*8;
        }
    }
    return -1;
}

unsigned int
akl_vector_add(struct akl_vector *vec, void *data)
{
    unsigned int i;
    void *at;
    assert(vec);
    i = akl_vector_get_free(vec);
    if (i == -1) {
        return akl_vector_push(vec, data);
    }
    at = akl_vector_at(vec, (unsigned int) i);
    memcpy(at, data, vec->av_msize);
    akl_vector_set_mark(vec, (unsigned int) i);
    vec->av_count++;
    return i;
}

void *akl_vector_find(struct akl_vector *vec
      , bool_t (*finder)(void *, void *), void *arg, unsigned int *ind)
{
    unsigned int i;
    void *at;
    assert(vec);
    for (i = 0; i < vec->av_size; i++) {
        at = akl_vector_at(vec, i);
        if (finder(at, arg)) {
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

    if (vec->av_free) {
        bmap = CALLOC_FUNCTION(vec->av_size/8+1, sizeof(akl_byte_t));
        memcpy(bmap, vec->av_free, osize/8+1);
        FREE_FUNCTION(vec->av_free);
        vec->av_free = bmap;
    }
}

/* The user should zero out the removed memory */
void *akl_vector_remove(struct akl_vector *vec, unsigned int ind)
{
    void *data;
    assert(vec);
    if (vec->av_size < ind)
        return NULL;

    akl_vector_init_free(vec);
    data = akl_vector_at(vec, ind);
    akl_vector_clear_mark(vec, ind);
    vec->av_count--;
    return data;
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
akl_register_type(struct akl_state *s, const char *name, akl_destructor_t de_fun)
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
    int ind;
    utype = akl_vector_find(&in->ai_utypes, utype_finder_name
                                        , (void *)tname, &ind);
    if (utype)
        return ind;

    return -1;
}
