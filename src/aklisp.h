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
#ifndef AKLISP_H
#define AKLISP_H
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "akl_tree.h"

#define AKL_MALLOC(s, type) (type *)akl_alloc(s, sizeof(type))
#define AKL_FREE(s, obj) (type *)akl_free(s, (void *)obj)

#define AKL_CHECK_TYPE(v1, type) (((v1) && (v1)->va_type == (type)) ? TRUE : FALSE)
#define AKL_GET_VALUE_MEMBER_PTR(val, type, member) \
                            ((AKL_CHECK_TYPE(val, type) \
                            ? (val)->va_value.member : NULL))

#define AKL_GET_VALUE_MEMBER(val, type, member) \
                            ((AKL_CHECK_TYPE(val, type) \
                            ? (val)->va_value.member : 0))

#define AKL_GET_ATOM_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, TYPE_ATOM, atom))
#define AKL_GET_NUMBER_VALUE(val) (AKL_GET_VALUE_MEMBER(val, TYPE_NUMBER, number))
#define AKL_GET_STRING_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, TYPE_STRING, string))
#define AKL_GET_CFUN_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, TYPE_CFUN, cfunc))
#define AKL_GET_LIST_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, TYPE_LIST, list))
struct akl_list;
struct akl_atom;
struct akl_value;
struct akl_io_device;
struct akl_state;
struct akl_vector;
enum AKL_VALUE_TYPE {
    TYPE_NIL,
    TYPE_ATOM,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_LIST,
    TYPE_TRUE,
    TYPE_FUNCTION,
    TYPE_USERDATA
};
typedef enum { FALSE, TRUE } bool_t;
typedef enum { DEVICE_FILE, DEVICE_STRING } device_type_t;
/* Ordinary C functions a.k.a. normal S-expressions*/
typedef void*(*akl_cfun_t)(struct akl_state *);
/* Special C functions a.k.a. special forms */
typedef struct akl_value*(*akl_scfun_t)(struct akl_state *, struct akl_list *);
typedef unsigned int akl_utype_t;
typedef unsigned char akl_byte_t;
typedef int (*akl_cmp_fn_t)(void *, void *);

#ifndef __unused
#define __unused __attribute__((unused))
#endif // __unused

#define AKL_IS_NIL(type)    ((type) == NULL || (type)->is_nil)
#define AKL_IS_QUOTED(type) ((type)->is_quoted)
#define AKL_IS_TRUE(type)   (!AKL_IS_NIL(type))
/*
 * This section contains the most important data structures
 * for a generational mark-and-sweep garbage collector.
 */
/* Size of the GC pools, must be the power of 2 */
#define AKL_GC_POOL_SIZE 64
#define AKL_GC_DEFINE_OBJ       struct akl_gc_object gc_obj
#define AKL_GC_MARKER(obj, m)   ((obj)->gc_obj.gc_mark_fun(obj, m))
#define AKL_GC_SET_MARK(obj, m) ((obj)->gc_obj.gc_mark = m)
#define AKL_GC_MARK(obj)        AKL_GC_MARKER(obj, TRUE)
#define AKL_GC_UNMARK(obj)      AKL_GC_MARKER(obj, FALSE)
#define AKL_GC_IS_MARKED(obj)   ((obj)->gc_obj.gc_mark == TRUE)
#define AKL_GC_SET_STATIC(obj)  ((obj)->gc_obj.gc_static = TRUE)
#define AKL_GC_INIT_OBJ(obj, de_fun, mk_fun) \
    ((obj)->gc_obj.gc_de_fun = de_fun; (obj)->gc_obj.gc_mark_fun = mk_fun; \
    (obj)->gc_obj.gc_mark = FALSE ; (obj)->gc_obj.gc_le_is_obj = FALSE;)


typedef void (*akl_gc_destructor_t)(struct akl_state *, void *obj);
/* Mark and unmark GC'd objects */
typedef void (*akl_gc_marker_t)(void *, bool_t);
/*
 * Every structure, which is suitable for collection, have to
 * embed this structure as the first variable, with the name of 'gc_obj'.
 * BE AWARE: This is object oriented! :-)
*/
struct akl_gc_object {
    /* Destructor function for the object getting a pointer
     of the active state and the object pointer as parameters. */
    akl_gc_destructor_t gc_de_fun;
    akl_gc_marker_t     gc_mark_fun;
    bool_t              gc_mark      : 1;
    /* Are the le_data field points to a GC-managed object? (Mostly used
     * for akl_list_entry structures) */
    bool_t              gc_le_is_obj : 1;
    bool_t              gc_static    : 1;
};

/* Just used for conversion purposes */
struct akl_gc_generic_object {
    AKL_GC_DEFINE_OBJ;
};

struct akl_userdata {
    unsigned int ud_id; /* Exact user type */
    void        *ud_private; /* Arbitrary userdata */
};

struct akl_lex_info {
    const char  *li_name;
    unsigned int li_line; /* Line count */
    unsigned int li_count; /* Column count */
};

enum AKL_FUNCTION_TYPE {
    AKL_FUNC_CFUN,
    AKL_FUNC_SPECIAL,
    AKL_FUNC_USER,
    AKL_FUNC_LAMBDA
};

struct akl_function {
    AKL_GC_DEFINE_OBJ;
    enum AKL_FUNCTION_TYPE fn_type;
    /* Body of the function */
    union {
        struct akl_list *lambda;
        akl_cfun_t  cfun;
        akl_scfun_t scfun;
    } fn_body;
    /* Argument count */
#define AKL_ARG_OPTIONAL -1
#define AKL_ARG_REST     -2
    struct akl_context *fn_ctx;
};

extern struct akl_value {
    AKL_GC_DEFINE_OBJ;
    struct akl_lex_info *va_lex_info;
    enum AKL_VALUE_TYPE  va_type;
    union {
		double number;
        struct akl_atom *atom;
        char  *string;
        struct akl_function *func;
        struct akl_userdata *udata;
		struct akl_list     *list;
    } va_value;

    bool_t is_quoted : 1;
    bool_t is_nil    : 1;
} NIL_VALUE, TRUE_VALUE;

struct akl_atom {
    AKL_GC_DEFINE_OBJ;
    struct akl_value  *at_value;
    char *at_name;
    char *at_desc; /* Documentation (mostly functions) */
    RB_ENTRY(akl_atom) at_entry;
    bool_t at_is_const : 1;  /* Is a constant atom? */
    /* We must need to know, the the constness of the
    strings too (at_name & at_desc too).
    free() or not to free(), this is the question? */
    bool_t at_is_cdef : 1;
};

/* To properly handle both file and string sources, we
  must have to create an abstraction for the Lexical Analyzer*/
struct akl_io_device {
    device_type_t iod_type;
    union {
        FILE       *file;
        const char *string;
    } iod_source;
    const char *iod_name; /* Name of the input device (for ex.: name of the file) */
    size_t      iod_pos; /*  Only used for DEVICE_STRING */

    char *iod_buffer; /* Lexer buffer */
    unsigned int iod_buffer_size;
    unsigned int iod_char_count;
    unsigned int iod_line_count;
    unsigned int iod_column;
};

/* NOTE: If you want to modify the following enumeration, you have
 * to modify the akl_gc_obj_sizes[] the same way,  too. */
#define AKL_GC_NR_TYPE  6
enum AKL_GC_OBJECT_TYPE {
       AKL_GC_VALUE = 0,
       AKL_GC_ATOM,
       AKL_GC_LIST,
       AKL_GC_LIST_ENTRY,
       AKL_GC_FUNCTION,
       AKL_GC_UDATA
};

const size_t akl_gc_obj_sizes[] = {
        sizeof(struct akl_value)
      , sizeof(struct akl_atom)
      , sizeof(struct akl_list)
      , sizeof(struct akl_list_entry)
      , sizeof(struct akl_function)
      , sizeof(struct akl_userdata)
};

struct akl_utype {
    const char *ut_name;
    akl_utype_t ut_id;
    akl_gc_destructor_t ut_de_fun; /* Destructor for the given type */
};

typedef int (*akl_mod_load_t)(struct akl_state *);
struct akl_module {
    const char *am_path;
    const char *am_name;
    const char *am_desc; /* Text description of the module */
    const char *am_author;
    void *am_handle; /* dlopen's handle */
    akl_mod_load_t am_load;
    akl_mod_load_t am_unload;
};
enum AKL_MOD_STATUS { AKL_LOAD_FAIL = 0, AKL_LOAD_OK  };
/* The first three arguments cannot be NULL */
#define AKL_MODULE_DEFINE(load, unload, name, desc, author) \
struct akl_module __module_desc = { \
    .am_name = name, \
    .am_path = NULL, \
    .am_author = author, \
    .am_desc = desc, \
    .am_load = load, \
    .am_unload = unload \
}

#define AKL_VECTOR_DEFSIZE 10
#define AKL_VECTOR_NEW(s, type, count) akl_vector_new(s, sizeof(type), count)
#define AKL_VECTOR_FOREACH(ind, ptr, vec) \
    for ((ind) = 0, (ptr) = akl_vector_at(vec, 0) \
        ;(ind) < akl_vector_count(vec); (ind)++, (ptr) = akl_vector_at(vec, ind))

/* Foreach on all elements including the NULL ones */
#define AKL_VECTOR_FOREACH_ALL(ind, ptr, vec) \
    for ((ind) = 0, (ptr) = akl_vector_at(vec, 0) \
        ;(ind) < akl_vector_size(vec); (ind)++, (ptr) = akl_vector_at(vec, ind))

struct akl_vector {
    void        *av_vector; /* Array of objects */
    unsigned int av_count;  /* Count of the added objects */
    unsigned int av_size;   /* Size of the array */
    unsigned int av_msize;  /* Size of the members in the array */
    /* Full_size = av_size * av_msize; see calloc() */
};


inline unsigned int akl_vector_size(struct akl_vector *);
inline unsigned int akl_vector_count(struct akl_vector *);
struct akl_vector  *akl_vector_new(struct akl_state *, unsigned int);
void         akl_vector_init(struct akl_state *, struct akl_vector *, unsigned int, unsigned int);
void        *akl_vector_reserve(struct akl_vector *);
unsigned int akl_vector_add(struct akl_vector *, void *);
unsigned int akl_vector_push(struct akl_vector *, void *);
bool_t       akl_vector_is_grow_need(struct akl_vector *vec);
void        *akl_vector_remove(struct akl_vector *, unsigned int);
void        *akl_vector_pop(struct akl_vector *);
void        *akl_vector_find(struct akl_vector *, akl_cmp_fn_t, void *, unsigned int *);
void         akl_vector_push_vec(struct akl_vector *, struct akl_vector *);
inline void *akl_vector_at(struct akl_vector *, unsigned int);
inline void *akl_vector_last(struct akl_vector *);
inline void *akl_vector_first(struct akl_vector *);
bool_t       akl_vector_is_empty_at(struct akl_vector *, unsigned int);
void         akl_vector_destroy(struct akl_vector *);
void         akl_vector_free(struct akl_vector *);
void         akl_vector_grow(struct akl_vector *);

struct akl_gc_pool {
    enum AKL_GC_OBJECT_TYPE gp_type;
    struct akl_vector       gp_pool;
    struct akl_gc_pool     *gp_next;
    /* Bitmap of the free slots */
    unsigned int            gp_freemap[AKL_GC_POOL_SIZE/(sizeof(unsigned int)*8)];
};

void   akl_stack_push(struct akl_state *, struct akl_value *);
struct akl_value *akl_stack_pop(struct akl_state *);
double akl_stack_pop_number(struct akl_state *);
char * akl_stack_pop_string(struct akl_state *);
struct akl_list *akl_stack_pop_list(struct akl_state *);
enum   AKL_VALUE_TYPE akl_stack_top_type(struct akl_state *);
struct akl_value *akl_stack_top(struct akl_state *);

typedef enum {
    AKL_NM_TERMINATE, AKL_NM_TRYAGAIN, AKL_NM_RETNULL
} akl_nomem_action_t;

/* An instance of the interpreter */
struct akl_state {
    void *(*ai_malloc_fn)(size_t);
    void *(*ai_calloc_fn)(size_t, size_t);
    void (*ai_free_fn)(void *);
    void (*ai_realloc_fn)(void *, size_t);
    akl_nomem_action_t (*ai_nomem_fn)(struct akl_state *);

    struct akl_io_device *ai_device;
    struct akl_list *ai_contexts;
    RB_HEAD(ATOM_TREE, akl_atom) ai_atom_head;
    unsigned int ai_gc_malloc_size; /* Totally malloc()'d bytes */
    /* Number of GC pools for diffrent types */
    unsigned int ai_gc_pool_count[AKL_GC_NR_TYPE];
    /* GC pool for each type */
    struct akl_gc_pool ai_gc_pool[AKL_GC_NR_TYPE];
    /* Last GC pool for the type */
    struct akl_gc_pool *ai_gc_pool_last[AKL_GC_NR_TYPE];
    struct akl_list *ai_program;
    /* Loaded user-defined types */
    struct akl_vector ai_utypes;
    /* Currently loaded modules */
    struct akl_list ai_modules;
    struct akl_list *ai_errors; /* Collection of the errors (if any, default NULL) */
    struct akl_vector ai_stack;
    struct akl_vector ai_ir_code;
    bool_t ai_interactive : 1;
    bool_t ai_gc_is_enabled : 1;
};

struct akl_label *akl_new_branches(struct akl_state *, struct akl_vector *v, unsigned int cnt);

struct akl_label {
    struct akl_vector *ab_branch;
    unsigned int       ab_code; /* The index of the first instruction */
};

enum AKL_CMP_TYPE {
    AKL_CMP_EQ = 0x1,
    AKL_CMP_GT = 0x2,
    AKL_CMP_LT = 0x4
};

enum AKL_IR_OPERATION {
    AKL_IR_NOP = 0,
    AKL_IR_STORE,
    AKL_IR_STORE_NIL,
    AKL_IR_STORE_TRUE,
    AKL_IR_LOAD,
    AKL_IR_CALL,
    AKL_IR_CMP,
    AKL_IR_JMP,
    AKL_IR_BRANCH,
    AKL_IR_GET,
    AKL_IR_SET
};

struct akl_ir_instruction {
    enum AKL_IR_OPERATIONS in_op;
    /* Only used for store, load and cmp */
    unsigned int in_argc;
    void        *in_arg[2];
};

static inline int cmp_atom(struct akl_atom *f, struct akl_atom *s)
{
    return strcasecmp(f->at_name, s->at_name);
}

RB_PROTOTYPE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

struct akl_atom *akl_add_builtin(struct akl_state *, const char *, akl_cfun_t, const char *);
struct akl_atom *akl_add_global_cfun(struct akl_state *, const char *, akl_cfun_t, const char *);
void   akl_remove_function(struct akl_state *, akl_cfun_t);
struct akl_atom *akl_get_global_atom(struct akl_state *in, const char *);
struct akl_atom *akl_add_global_atom(struct akl_state *, struct akl_atom *);
void   akl_do_on_all_atoms(struct akl_state *, void (*fn)(struct akl_atom *));
void   akl_remove_global_atom(struct akl_state *, struct akl_atom *);
bool_t akl_is_equal_with(struct akl_atom *, const char **);

struct akl_list_entry {
    AKL_GC_DEFINE_OBJ;
    struct akl_list_entry *le_next;
    struct akl_list_entry *le_prev;
    void                  *le_data;
};

struct akl_list {
    AKL_GC_DEFINE_OBJ;
    struct akl_list_entry *li_head;
    struct akl_list_entry *li_last;
    struct akl_list *li_parent; /* Parent (container) list */
    unsigned int     li_elem_count;
    bool_t is_quoted : 1;
/* Yep element count == 0 can simply mean NIL, but
  then we cannot use the AKL_IS_NIL() macro :-( */
    bool_t is_nil    : 1;
};

#define AKL_LIST_FIRST(list) ((list)->li_head)
#define AKL_LIST_LAST(list) ((list)->li_last)
#define AKL_LIST_NEXT(ent) ((ent)->le_next)
#define AKL_LIST_PREV(ent) ((ent)->le_prev)
#define AKL_LIST_SECOND(list) (AKL_LIST_NEXT(AKL_LIST_FIRST(list)))
#define AKL_FIRST_VALUE(list) (struct akl_value *)akl_list_index(list, 0)
#define AKL_SECOND_VALUE(list) (struct akl_value *)akl_list_index(list, 1)
#define AKL_THIRD_VALUE(list) (struct akl_value *)akl_list_index(list, 2)
/* Forward  foreach, usage:
 *
 * void do_list(struct akl_list *list) {
 *   struct akl_list_entry *ent = NULL;
 *   AKL_LIST_FOREACH(ent, list) {
 *      do_something_value(AKL_ENTRY_VALUE(ent));
 *   }
 * }
*/
#define AKL_LIST_FOREACH(elem, list)   \
    for ((elem) = AKL_LIST_FIRST(list) \
       ; (elem)                        \
       ; (elem) = AKL_LIST_NEXT(elem))

#define AKL_LIST_FOREACH_BACK(elem, list) \
    for ((elem) = AKL_LIST_LAST(list)     \
       ; (elem)                           \
       ; (elem) = AKL_LIST_PREV(elem))

/* Sometimes, useful to traverse the list from the second element */
#define AKL_LIST_FOREACH_SECOND(elem, list) \
    for ((elem) = AKL_LIST_SECOND(list)     \
       ; (elem)                             \
       ; (elem) = AKL_LIST_NEXT(elem))

/* If, the elem pointer will be modified (i.e: free()'d) you
  must use this macro */
#define AKL_LIST_FOREACH_SAFE(elem, list, tmp)   \
    for ((elem) = AKL_LIST_FIRST(list)           \
       ; (elem) && ((tmp) = AKL_LIST_NEXT(elem)) \
       ; (elem) = (tmp))

#define AKL_ENTRY_VALUE(elem) (struct akl_value *)((elem)->le_data)

typedef enum {
    tEOF,
    tNIL,
    tATOM,
    tNUMBER,
    tSTRING,
    tTRUE,
    tLBRACE,
    tRBRACE,
    tQUOTE
} akl_token_t;

void *akl_alloc(struct akl_state *, size_t);
void *akl_calloc(struct akl_state *, size_t, size_t);
void *akl_realloc(struct akl_state *, void *, size_t);
void akl_free(struct akl_state *, void *);

struct akl_state     *akl_new_file_interpreter(const char *, FILE *, void (*)(size_t));
struct akl_state     *akl_new_string_interpreter(const char *, const char *, void (*)(size_t));
struct akl_io_device *akl_new_file_device(const char *, FILE *, void (*)(size_t));
struct akl_io_device *akl_new_string_device(const char *, const char *, void (*)(size_t));
struct akl_state     *akl_reset_string_interpreter(struct akl_state *, const char *, const char *, void (*)(size_t));
void                  akl_init_string_device(const char *name, const char *str);
void                  akl_init_string_device(const char *name, const char *str);

akl_token_t akl_lex(struct akl_io_device *);
void    akl_lex_free(struct akl_io_device *);
char   *akl_lex_get_string(struct akl_io_device *);
double  akl_lex_get_number(struct akl_io_device *);
char   *akl_lex_get_atom(struct akl_io_device *);

struct akl_list  *akl_parse_list(struct akl_state *, struct akl_io_device *);
struct akl_value *akl_parse_value(struct akl_state *, struct akl_io_device *);
struct akl_list  *akl_parse_file(struct akl_state *, const char *, FILE *);
struct akl_list  *akl_parse_string(struct akl_state *, const char *, const char *);
struct akl_list  *akl_parse_io(struct akl_state *, struct akl_io_device *);
struct akl_list  *akl_parse(struct akl_state *);
struct akl_value *akl_car(struct akl_list *);
struct akl_list  *akl_cdr(struct akl_state *, struct akl_list *);

/* Creating and destroying structures */
void                  *akl_init_state(struct akl_state *s);
struct akl_state      *akl_new_state(void *(malloc_fn)(void *));
void                   akl_init_list(struct akl_list *);
struct akl_list       *akl_new_list(struct akl_state *);
struct akl_atom       *akl_new_atom(struct akl_state *, char *);
struct akl_list_entry *akl_new_list_entry(struct akl_state *);
struct akl_value      *akl_new_value(struct akl_state *);
struct akl_value      *akl_new_string_value(struct akl_state *, char *);
struct akl_value      *akl_new_number_value(struct akl_state *, double);
struct akl_value      *akl_new_list_value(struct akl_state *, struct akl_list *);
struct akl_value      *akl_new_atom_value(struct akl_state *, char *);
struct akl_value      *akl_new_user_value(struct akl_state *, akl_utype_t, void *);
struct akl_lex_info   *akl_new_lex_info(struct akl_state *, struct akl_io_device *);

/* GC functions */

void   akl_gc_init(struct akl_state *);
void   akl_gc_mark(struct akl_state *);
void   akl_gc_mark_list(void *, bool_t);
void   akl_gc_mark_list_entry(void *, bool_t);
void   akl_gc_mark_atom(void *, bool_t);
void   akl_gc_mark_value(void *, bool_t);
void   akl_gc_sweep_pool(struct akl_gc_pool *);
void   akl_gc_sweep(struct akl_state *);
void   akl_gc_enable(struct akl_state *);
void   akl_gc_disable(struct akl_state *);
void   akl_gc_pool_add(struct akl_state *, void *, enum AKL_GC_OBJECT_TYPE);
struct akl_gc_pool *akl_new_gc_pool(struct akl_state *, enum AKL_GC_OBJECT_TYPE);
bool_t akl_gc_pool_is_empty(struct akl_gc_pool *);
bool_t akl_gc_pool_tryfree(struct akl_state *);

char             *akl_num_to_str(struct akl_state *, double);
struct akl_value *akl_to_number(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_string(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_atom(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_symbol(struct akl_state *, struct akl_value *);

akl_utype_t akl_get_utype_value(struct akl_value *);
char       *akl_get_atom_name_value(struct akl_value *);
void       *akl_get_udata_value(struct akl_value *);
struct      akl_userdata *akl_get_userdata_value(struct akl_value *);
bool_t      akl_check_user_type(struct akl_value *, akl_utype_t);
struct akl_module *akl_get_module_descriptor(struct akl_state *, struct akl_value *);

struct akl_list_entry *
akl_list_append(struct akl_state *, struct akl_list *, void *);
struct akl_list_entry *
akl_list_append_value(struct akl_state *, struct akl_list *, struct akl_value *);

void   akl_list_insert_head(struct akl_list *, struct akl_value *);
struct akl_list_entry *
akl_list_insert_value_head(struct akl_state *, struct akl_list *, struct akl_value *);
struct akl_value *akl_list_shift(struct akl_list *);

struct akl_value *akl_list_index_value(struct akl_list *, int);
struct akl_list_entry *akl_list_find(struct akl_list *, akl_cmp_fn_t, void *);
void   akl_list_remove_entry(struct akl_list *, struct akl_list_entry *);
struct akl_list_entry *
       akl_list_remove(struct akl_list *, akl_cmp_fn_t, void *);
struct akl_value *akl_duplicate_value(struct akl_state *, struct akl_value *);
struct akl_list *akl_list_duplicate(struct akl_state *, struct akl_list *);


int    akl_io_getc(struct akl_io_device *);
int    akl_io_ungetc(int, struct akl_io_device *);
bool_t akl_io_eof(struct akl_io_device *dev);

struct akl_value *akl_eval_value(struct akl_state *, struct akl_value *);
struct akl_value *akl_eval_list(struct akl_state *, struct akl_list *);
void   akl_eval_program(struct akl_state *);

void   akl_print_value(struct akl_state *, struct akl_value *);
void   akl_print_list(struct akl_state *, struct akl_list *);
int    akl_compare_values(void *, void *);
int    akl_get_typeid(struct akl_state *, const char *);

enum AKL_ALERT_TYPE {
   AKL_ERROR, AKL_WARNING
};

struct akl_error {
    struct akl_lex_info *err_info;
    enum AKL_ALERT_TYPE err_type;
    const char *err_msg;
};

void akl_add_error(struct akl_state *, enum AKL_ALERT_TYPE, struct akl_lex_info *, const char *fmt, ...);
void akl_clear_errors(struct akl_state *);
void akl_print_errors(struct akl_state *);

/* Create a new user type and register it for the interpreter. The returned
  integer will identify this new type. */
unsigned int akl_register_type(struct akl_state *, const char *, akl_gc_destructor_t);
void akl_deregister_type(struct akl_state *, unsigned int);

enum AKL_INIT_FLAGS {
    AKL_LIB_BASIC = 0x001,
    AKL_LIB_NUMBERIC = 0x002,
    AKL_LIB_CONDITIONAL = 0x004,
    AKL_LIB_PREDICATE = 0x008,
    AKL_LIB_DATA = 0x010,
    AKL_LIB_SYSTEM = 0x020,
    AKL_LIB_TIME = 0x040,
    AKL_LIB_OS = 0x080,
    AKL_LIB_LOGICAL = 0x100,
    AKL_LIB_FILE = 0x200,
    AKL_LIB_ALL = AKL_LIB_BASIC|AKL_LIB_NUMBERIC
        |AKL_LIB_CONDITIONAL|AKL_LIB_PREDICATE|AKL_LIB_DATA
        |AKL_LIB_SYSTEM|AKL_LIB_TIME|AKL_LIB_OS|AKL_LIB_LOGICAL|AKL_LIB_FILE
};

/* The GC must know the sizes for each
 collectable object */
const size_t gc_obj_sizes[] = {
    sizeof(struct akl_value),
    sizeof(struct akl_atom),
    sizeof(struct akl_list),
    sizeof(struct akl_list_entry),
    sizeof(struct akl_function),
    sizeof(struct akl_userdata)
};

void akl_init_lib(struct akl_state *, enum AKL_INIT_FLAGS);
void akl_init_os(struct akl_state *);
void akl_init_file(struct akl_state *);

#define AKL_CFUN_DEFINE(fname, iname, aname) \
    static struct akl_value * fname##_function(struct akl_state * iname, struct akl_list * aname)

#define AKL_BUILTIN_DEFINE(bname, iname, aname) \
    static struct akl_value * bname##_builtin(struct akl_state * iname, struct akl_list  * aname)

#define AKL_ADD_CFUN(in, fname, name, desc) \
    akl_add_global_cfun((in), (name), fname##_function, (desc))

#define AKL_ADD_BUILTIN(in, bname, name, desc) \
    akl_add_builtin((in), (name), bname##_builtin, (desc))

#define AKL_REMOVE_CFUN(in, fname) \
    akl_remove_function((in), fname##_function)

#define AKL_REMOVE_BUILTIN(in, bname) \
    akl_remove_function((in), bname##_builtin)


#if USE_COLORS
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"
#define GRAY   "\x1b[1;30m"
#define BLUE   "\x1b[34m"
#define RED    "\x1b[31m"
#define PURPLE "\x1b[35m"
#define BRIGHT_GREEN "\x1b[1;32m"
#define START_COLOR(c) printf("%s", (c))
#define END_COLOR_MARK "\x1b[0m"
#define END_COLOR printf("\x1b[0m")
#else
#define GREEN  ""
#define YELLOW ""
#define GRAY   ""
#define BLUE   ""
#define RED    ""
#define PURPLE ""
#define BRIGHT_GREEN ""
#define START_COLOR(c)
#define END_COLOR
#endif

#endif // AKLISP_H
