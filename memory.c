#include <stdlib.h>
#include "aklisp.h"

#define MALLOC_FUNCTION malloc
#define FREE_FUNCTION free

#define AKL_MALLOC(type) (type *)akl_malloc(sizeof(type))
#define AKL_FREE(type, ptr) FREE_FUNCTION((type *)(ptr))

void *akl_malloc(size_t size)
{
    void *ptr;
    ptr = MALLOC_FUNCTION(size);
    if (ptr == NULL) {
        fprintf(stderr, "ERROR! No memory left!\n");
        exit(-1);
    } else {
        return ptr;
    }
}

struct akl_instance *akl_new_instance(void)
{
    struct akl_instance *in = AKL_MALLOC(struct akl_instance);
    RB_INIT(&in->ai_atom_head);
    in->ai_list_count = 0;
    in->ai_atom_count = 0;
    in->ai_value_count = 0;
    in->ai_string_count = 0;
    in->ai_number_count = 0;
    in->ai_bool_count = 0;
    in->ai_list_entry_count = 0;
    in->ai_program = NULL;
    return in;
}

struct akl_list *akl_new_list(struct akl_instance *in)
{
    struct akl_list *lh = AKL_MALLOC(struct akl_list);
    lh->li_head = NULL;
    lh->li_last = NULL;
    lh->li_is_quoted = 0;
    lh->li_is_nil = 1; /* It has elements so it's NIL */
    lh->li_elem_count = 0;
    if (in != NULL)
        in->ai_list_count++;

    return lh;
}

struct akl_atom *akl_new_atom(struct akl_instance *in, char *name)
{
    struct akl_atom *atom = AKL_MALLOC(struct akl_atom);
    assert(name);
    atom->at_value = NULL;
    atom->at_name = name;
    if (in != NULL)
        in->ai_atom_count++;
    return atom;
}

struct akl_list_entry *akl_new_list_entry(struct akl_instance *in)
{
    struct akl_list_entry *ent = AKL_MALLOC(struct akl_list_entry);
    ent->le_value = NULL;
    ent->le_entry = NULL;
    if (in != NULL)
        in->ai_list_entry_count++;

    return ent;
}

void akl_free_list_entry(struct akl_instance *in, struct akl_list_entry *ent)
{
    if (ent == NULL)
        return;

    akl_free_value(in, AKL_ENTRY_VALUE(ent));
    if (in != NULL)
        in->ai_list_entry_count--;

    AKL_FREE(struct akl_list_entry, ent);
}

struct akl_value *akl_new_value(struct akl_instance *in)
{
    struct akl_value *val = AKL_MALLOC(struct akl_value);
    val->is_nil = 0;
    val->is_quoted = 0;
    return val;
}

void akl_free_value(struct akl_instance *in, struct akl_value *val)
{
    if (val != NULL) {
        switch (val->va_type) {
            case TYPE_LIST:
                akl_free_list(in, akl_get_list_value(val));
            break

            case TYPE_ATOM:
                akl_free_atom(in, akl_get_atom_value(val));
            break;

            case TYPE_STRING:
            /* No need for akl_free_string_value()  */
                AKL_FREE(char, akl_get_string_value(val));
                in && in->ai_string_count--;
            break;

            /*  Nothing to do */
            case TYPE_CFUN: case TYPE_BOOL:
            break;
            case TYPE_NUMBER:
                in && in->ai_number_count--;
            break;
        }
        AKL_FREE(struct akl_value, val);
    }
}

struct akl_value *akl_new_string_value(struct akl_instance *in, char *str)
{
    struct value *val = new_value(in);
    assert(str != NULL);
    val->va_type = TYPE_STRING;
    val->va_value.string = str;
    if (in != NULL)
        in->ai_string_count--;

    return val;
}

struct akl_value *akl_new_number_value(struct akl_instance *in, int num)
{
    struct value *val = new_value(in);
    val->va_type = TYPE_NUMBER;
    val->va_value.number = num;
    if (in != NULL)
        in->ai_number_count--;

    return val;
}

struct akl_value *akl_new_list_value(struct akl_instance *in, struct akl_list *lh)
{
    struct value *val = new_value();
    assert(lh != NULL);
    val->va_type = TYPE_LIST;
    val->va_value.list = lh;
    if (in != NULL)
        in->ai_list_count++;

    return val;
}

void akl_free_list(struct akl_instance *in, struct akl_list *list)
{
    struct akl_list_entry *ent, tmp;
    if (list == NULL)
        return;

    AKL_LIST_FOREACH_SAFE(ent, list, tmp) {
        akl_free_list_entry(ent);
    }
    if (in != NULL)
        in->ai_list_count--;

    AKL_FREE(struct akl_list, list);
}

struct akl_value *
akl_new_atom_value(struct akl_instance *in, char *name)
{
    struct akl_value *val = akl_new_value(in);
    assert(name != NULL);
    struct akl_atom *atm = akl_new_atom(in, name);
    val->va_type = TYPE_ATOM;
    val->va_value.atom = atm;
    return val;
}

void akl_free_atom(struct akl_instance *in, struct akl_atom *atom)
{
    if (atom == NULL)
        return;

    AKL_FREE(char, atom->at_name);
    akl_free_value(atom->at_value);
    AKL_FREE(struct akl_atom, atom);
    if (in != NULL)
        in->ai_atom_count--;
}

/*
 * The following functions needed, to get different
 * values from a given akl_value *. They must be functions
 * to easily evaulate pointers and value types.
*/
struct akl_atom *
akl_get_atom_value(struct akl_value *val)
{
    if (val != NULL && val->va_type == TYPE_ATOM
        && val->va_value.atom != NULL)
        return val->va_value.atom;
    return NULL;
}

int *akl_get_number_value(struct akl_value *val)
{
    if (val != NULL && val->va_type == TYPE_NUMBER)
        return &val->va_value.number;
    return NULL;
}

char *akl_get_string_value(struct akl_value *val)
{
    if (val != NULL && val->va_type == TYPE_STRING)
        return val->va_value.string;
    return NULL;
}

akl_cfun_t akl_get_cfun_value(struct akl_value *val)
{
    if (val != NULL && val->va_type == TYPE_CFUN)
        return &val->va_value.string;
    return NULL;
}

char *akl_get_atom_name_value(struct akl_value *val)
{
    struct akl_atom *atom = akl_get_atom_value(val);
    if (atom != NULL) {
        return atom->at_name;
    }
    return NULL;
}

struct akl_list_entry *
akl_list_append(struct akl_instance *in, struct akl_list *list, struct value *val)
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
    return le; 
}

struct akl_io_device *
akl_new_file_device(FILE *fp)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(akl_io_device);
    dev->iod_type = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos = 0;
    dev->iod_strlen = 0;
    return dev;
}

struct akl_io_device *
akl_new_string_device(const char *str)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(akl_io_device);
    dev->iod_type = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos = 0;
    dev->iod_strlen = strlen(str);
    return dev;
}
