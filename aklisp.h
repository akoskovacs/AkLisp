#ifndef AKLISP_H
#define AKLISP_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tree.h"

#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_ADDITIONAL "alpha"

#define MALLOC_FUNCTION malloc
#define FREE_FUNCTION free
#define AKL_MALLOC(type) (type *)akl_malloc(sizeof(type))
#define AKL_FREE(ptr) FREE_FUNCTION((void *)(ptr))

struct akl_list;
struct akl_atom;
struct akl_value;
struct akl_io_device;
struct akl_instance;
enum type { TYPE_NIL, TYPE_ATOM, TYPE_NUMBER, TYPE_STRING, TYPE_LIST, TYPE_TRUE, TYPE_CFUN };
typedef enum { FALSE, TRUE } bool_t;
typedef enum { DEVICE_FILE, DEVICE_STRING } device_type_t;
typedef struct akl_value*(*akl_cfun_t)(struct akl_instance *, struct akl_list *);

#ifndef __unused
#define __unused __attribute__((unused))
#endif // __unused

#define AKL_IS_NIL(type) ((type)->is_nil)
#define AKL_IS_QUOTED(type) ((type)->is_quoted)
#define AKL_IS_TRUE(type) (!IS_NIL(type))

static struct akl_value {
    enum type va_type;
    union {
        struct akl_atom *atom;
        char *string;
        int number;
        struct akl_list *list;
        akl_cfun_t cfunc;
    } va_value;

    bool_t is_quoted : 1;
    bool_t is_nil : 1;
} NIL_VALUE = {
    .va_type = TYPE_NIL,
    .va_value.number = 0,
    .is_quoted = TRUE, 
    .is_nil = TRUE,
};

static struct akl_value TRUE_VALUE = {
    .va_type = TYPE_TRUE,
    .va_value.number = 1,
    .is_quoted = TRUE,
    .is_nil = FALSE,
};

struct akl_atom {
    RB_ENTRY(akl_atom) at_entry;
    struct akl_value *at_value;
    char *at_name;
    char *at_desc; /* Documentation (mostly functions) */
};

/* To properly handle both file and string sources, we
  must have to create an abstraction for the Lexical Analyzer*/
struct akl_io_device {
    device_type_t iod_type;
    union {
        FILE *file;
        const char *string;
    } iod_source;
    off_t iod_pos; /*  Only used for DEVICE_STRING */
};

struct akl_instance {
    struct akl_io_device *ai_device;
    RB_HEAD(ATOM_TREE, akl_atom) ai_atom_head;
    unsigned int ai_list_count;
    unsigned int ai_list_entry_count;
    unsigned int ai_atom_count;
    unsigned int ai_value_count;
    unsigned int ai_string_count;
    unsigned int ai_number_count;
    unsigned int ai_bool_count;
    struct akl_list *ai_program;
};

static inline int cmp_atom(struct akl_atom *f, struct akl_atom *s)
{
    return strcasecmp(f->at_name, s->at_name);
}

RB_PROTOTYPE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

void akl_add_global_atom(struct akl_instance *, struct akl_atom *);
struct akl_atom *akl_add_global_cfun(struct akl_instance *, const char *, akl_cfun_t, const char *);
struct akl_atom *akl_get_global_atom(struct akl_instance *in, const char *);
void akl_do_on_all_atoms(struct akl_instance *, void (*fn)(struct akl_atom *));

struct akl_list_entry {
    struct akl_value *le_value;
    struct akl_list_entry *le_next;
};

struct akl_list {
    struct akl_list_entry *li_head;
    struct akl_list_entry *li_last;
    int li_elem_count;
    bool_t is_quoted : 1;
/* Yep element count == 0 can simply mean NIL, but
  then we cannot use the AKL_IS_NIL() macro :-( */
    bool_t is_nil : 1;
} NIL_LIST;

#define AKL_LIST_FIRST(list) ((list)->li_head)
#define AKL_LIST_LAST(list) ((list)->li_last)
#define AKL_LIST_NEXT(ent) ((ent)->le_next)
#define AKL_LIST_SECOND(list) (AKL_LIST_NEXT(AKL_LIST_FIRST(list)))
#define AKL_LIST_FOREACH(elem, list)  \
    for ((elem) = AKL_LIST_FIRST(list)\
       ; (elem)                        \
       ; (elem) = AKL_LIST_NEXT(elem))

#define AKL_LIST_FOREACH_SAFE(elem, list, tmp)  \
    for ((tmp) = (elem) = AKL_LIST_FIRST(list) \
       ; (tmp)                                  \
       ; (tmp) = (elem) = AKL_LIST_NEXT(tmp))

#define AKL_ENTRY_VALUE(elem) ((elem)->le_value)

typedef enum {
    tEOF,
    tNIL,
    tATOM,
    tNUMBER,
    tSTRING,
    tTRUE,
    tLBRACE = '(',
    tRBRACE = ')',
    tQUOTE = '\''
} token_t;

struct akl_instance  *akl_new_file_interpreter(FILE *);
struct akl_instance  *akl_new_string_interpreter(const char *);
struct akl_io_device *akl_new_file_device(FILE *);
struct akl_io_device *akl_new_string_device(const char *);

token_t akl_lex(struct akl_io_device *);
char   *akl_lex_get_string(void);
int     akl_lex_get_number(void);
char   *akl_lex_get_atom(void);

struct akl_list *
akl_parse_list(struct akl_instance *, struct akl_io_device *, bool_t);
struct akl_list  *akl_parse_file(struct akl_instance *, FILE *fp);
struct akl_list  *akl_parse_string(struct akl_instance *, const char *);
struct akl_list  *akl_parse_io(struct akl_instance *);
struct akl_value *akl_car(struct akl_list *l);
struct akl_list  *akl_cdr(struct akl_instance *, struct akl_list *l);

/* Creating and destroying structures */
struct akl_instance   *akl_new_instance(void);
struct akl_list       *akl_new_list(struct akl_instance *in);
struct akl_atom       *akl_new_atom(struct akl_instance *in, char *name);
struct akl_list_entry *akl_new_list_entry(struct akl_instance *in);
struct akl_value      *akl_new_value(struct akl_instance *in);
struct akl_value      *akl_new_string_value(struct akl_instance *in, char *str);
struct akl_value      *akl_new_number_value(struct akl_instance *in, int num);
struct akl_value      *akl_new_list_value(struct akl_instance *in, struct akl_list *lh);
struct akl_value      *akl_new_atom_value(struct akl_instance * in, char *name);

void akl_free_atom(struct akl_instance *in, struct akl_atom *atom);
void akl_free_value(struct akl_instance *in, struct akl_value *val);
void akl_free_list_entry(struct akl_instance *in, struct akl_list_entry *ent);
void akl_free_list(struct akl_instance *in, struct akl_list *list);
void akl_free_instance(struct akl_instance *in);

struct akl_list_entry *
akl_list_append(struct akl_instance *, struct akl_list *, struct akl_value *);
struct akl_value *akl_list_index(struct akl_list *, int);
struct akl_list_entry *akl_list_find(struct akl_list *, struct akl_value *);
struct akl_value *akl_entry_to_value(struct akl_list_entry *);
/*  Getting back values */
struct akl_atom *akl_get_atom_value(struct akl_value *);
struct akl_list *akl_get_list_value(struct akl_value *);
/* WARNING: It is also a pointer! */
int             *akl_get_number_value(struct akl_value *);
char            *akl_get_string_value(struct akl_value *);
akl_cfun_t       akl_get_cfun_value(struct akl_value *);
char            *akl_get_atom_name_value(struct akl_value *);

int    akl_io_getc(struct akl_io_device *);
int    akl_io_ungetc(int, struct akl_io_device *);
bool_t akl_io_eof(struct akl_io_device *dev);

void print_value(struct akl_value *);
void print_list(struct akl_list *);

void init_lib(struct akl_instance *in);
#endif // AKLISP_H
