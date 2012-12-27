#include "aklisp.h"

/* ~~~===### VECTOR ###===~~~ */
struct akl_vector *
akl_vector_new(struct akl_state *s, unsigned int vsize
                                  , unsigned int defsize)
{
    struct akl_vector *vec = AKL_MALLOC(s, struct akl_vector);
    if (!defsize)
        defsize = AKL_VECTOR_DEFSIZE;

    akl_vector_init(vec, vsize, defsize);
    return vec;
}

void akl_vector_init(struct akl_vector *vec, unsigned int vsize
                                  , unsigned int defsize)
{
    assert(vec);
    vec->av_vector = CALLOC_FUNCTION(defsize, vsize);
    vec->av_size = defsize;
    vec->av_count = 0;
}

void *akl_vector_pop(struct akl_vector *vec)
{
    assert(vec);
    if (vec->av_count == 0) 
        return NULL;

    return vec->av_vector[vec->av_count--];
}

unsigned int 
akl_vector_push(struct akl_vector *vec, void *data)
{
    unsigned int ind;
    assert(vec);
    if (vec->av_count < vec->av_size)
        akl_vector_grow(vec);

    ind = vec->av_count;
    vec->av_vector[ind] = data;
    vec->av_count++;
    return ind;
}

unsigned int 
akl_vector_add(struct akl_vector *vec, void *data)
{
    unsigned int i;
    assert(vec);
    for (i = 0; i < vec->av_count; i++) {
        if (vec->av_vector[i] == NULL) {
            vec->av_vector[i] = data;
            vec->av_count++;
            return i;
        }
    }
    return akl_vector_push(vec, data);
}

void *akl_vector_find(struct akl_vector *vec
      , bool_t (*finder)(void *, void *), void *arg, unsigned int *ind)
{
    unsigned int i;
    assert(vec);
    for (i = 0; i < vec->av_count; i++) {
        if (finder(vec->av_vector[i], arg)) {
            if (ind != NULL)
                *ind = i;
            return vec->av_vector[i];
        }
    }
    return NULL;
}

void akl_vector_grow(struct akl_vector *vec)
{
    assert(vec);
    if (vec->av_size > 30)
        vec->av_size = vec->av_size + (vec->av_size/2);
    else
        vec->av_size = vec->av_size * 2;
    vec->av_vector = REALLOC_FUNCTION(vec->av_vector, vec->av_size);
}

void *akl_vector_remove(struct akl_vector *vec, unsigned int ind)
{
    void *data;
    assert(vec);
    if (vec->av_size < ind)
        return NULL;

    data = vec->av_vector[ind];
    vec->av_vector[ind] = NULL;
    vec->av_count--;
    return data;
}

inline void *
akl_vector_at(struct akl_vector *vec, unsigned int ind)
{
    assert(vec);
    if (ind < vec->av_count) {
        return vec->av_vector[ind];
    }
    return NULL;
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

/* ~~~===### Some getters ###===~~~ */

char *akl_get_atom_name_value(struct akl_value *val)
{
    struct akl_atom *atom = AKL_GET_ATOM_VALUE(val);
    return (atom != NULL) ? atom->at_name : NULL;
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
    utype = akl_vector_find(&in->ai_utypes, utype_finder_name, tname, &ind);
    if (utype)
        return ind;

    return -1;
}
