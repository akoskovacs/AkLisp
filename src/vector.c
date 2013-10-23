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
akl_new_vector(struct akl_state *s, unsigned int nmemb, unsigned int size)
{
    struct akl_vector *vec = AKL_MALLOC(s, struct akl_vector);
    akl_init_vector(s, vec, nmemb, size);
    return vec;
}

void akl_init_vector(struct akl_state *s, struct akl_vector *vec, unsigned int nmemb, unsigned int size)
{
    assert(vec);
    if (!nmemb)
        nmemb = AKL_VECTOR_DEFSIZE;

    if (!size)
        size = sizeof(void *);

    vec->av_vector = (char *)akl_calloc(s, nmemb, size);
    vec->av_size = size;
    vec->av_count = 0;
    vec->av_msize = nmemb;
    vec->av_state = s;
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
    assert(vec);
    if (akl_vector_is_grow_need(vec))
        akl_vector_grow(vec, 0);

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

void akl_vector_set(struct akl_vector *vec, unsigned int w, void *data)
{
    if (w > vec->av_count) {

    }
    void *ptr = vec->av_vector + (w*vec->av_msize);
    memcpy(ptr, data, vec->av_size);
}

void akl_vector_push_vec(struct akl_vector *vec, struct akl_vector *v)
{
    void *ptr;
    if (vec->av_msize != v->av_msize)
        return;

    if (vec->av_count + v->av_count + 1 < vec->av_size) {
        akl_vector_grow(vec, 0);
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
    for (i = 0; i < vec->av_count; i++) {
        at = akl_vector_at(vec, i);
        if (!cmp_fn(at, arg)) {
            if (ind != NULL)
                *ind = i;
            return at;
        }
    }
    return NULL;
}

void akl_vector_grow(struct akl_vector *vec, unsigned int to)
{
    assert(vec);
    if (to != 0 && to > vec->av_size)
        vec->av_size = to;
    else
        vec->av_size = vec->av_size + (vec->av_size/2);

    vec->av_vector = akl_realloc(vec->av_state, vec->av_vector
                                    , vec->av_size*vec->av_msize);
}

/* The user should zero out the removed memory */
void *akl_vector_remove(struct akl_vector *vec, unsigned int ind)
{
    /* TODO */
    return NULL;
}

void *
akl_vector_at(struct akl_vector *vec, unsigned int ind)
{
    assert(vec);
    if (!akl_vector_is_empty(vec) && ind < vec->av_size) {
        return vec->av_vector + (ind * vec->av_msize);
    }
    return NULL;
}

void *
akl_vector_last(struct akl_vector *vec)
{
    return akl_vector_at(vec, vec->av_count);
}

void *
akl_vector_first(struct akl_vector *vec)
{
    return vec->av_vector;
}

unsigned int
akl_vector_size(struct akl_vector *vec)
{
    assert(vec);
    return vec->av_size;
}

unsigned int
akl_vector_count(struct akl_vector *vec)
{
    assert(vec);
    return vec->av_count;
}

void akl_vector_destroy(struct akl_state *s, struct akl_vector *vec)
{
    akl_free(s, vec->av_vector, vec->av_size);
}

void akl_vector_free(struct akl_state *s, struct akl_vector *vec)
{
    akl_vector_destroy(s, vec);
    AKL_FREE(s, vec);
}
