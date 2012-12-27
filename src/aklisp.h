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

#define MALLOC_FUNCTION malloc
#define FREE_FUNCTION free
#define AKL_MALLOC(in, type) (type *)akl_malloc(in, sizeof(type))
#define AKL_FREE(ptr) FREE_FUNCTION((void *)(ptr))

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
enum akl_type { 
    TYPE_NIL, 
    TYPE_ATOM,
    TYPE_NUMBER, 
    TYPE_STRING,
    TYPE_LIST,
    TYPE_TRUE,
    TYPE_CFUN,
    TYPE_BUILTIN,
    TYPE_USERDATA
};
typedef enum { FALSE, TRUE } bool_t;
typedef enum { DEVICE_FILE, DEVICE_STRING } device_type_t;
typedef struct akl_value*(*akl_cfun_t)(struct akl_state *, struct akl_list *);
typedef unsigned int akl_utype_t;

#ifndef __unused
#define __unused __attribute__((unused))
#endif // __unused

#define AKL_IS_NIL(type) ((type) == NULL || (type)->is_nil)
#define AKL_IS_QUOTED(type) ((type)->is_quoted)
#define AKL_IS_TRUE(type) (!AKL_IS_NIL(type))

/* This section contains the most important datastructures
 for the garbage collector */
typedef void (*akl_destructor_t)(struct akl_state *, void *obj);
/* Every structure, which is suitable for collection, have to
   embed this structure as a 'gc_obj' variable.
   BE AWARE: This is object orineted! :-) */
struct akl_gc_object {
    /* Reference count for the object */
    unsigned int gc_ref_count;
    /* Desctructor function for the object getting a pointer 
     of the active state and the object pointer as parameters. */
    akl_destructor_t gc_de_fun;
    bool_t gc_is_static : 1; /* Static object will not be free()'d */
};
#define AKL_GC_DEFINE_OBJ struct akl_gc_object gc_obj
#define AKL_GC_INIT_OBJ(obj, de_fun) (obj)->gc_obj.gc_ref_count = 0; \
                                     (obj)->gc_obj.gc_is_static = FALSE; \
                                     (obj)->gc_obj.gc_de_fun = de_fun
/* Call the destructor */
#define AKL_GC_COLLECT_OBJ(in, obj) (obj)->gc_obj.gc_de_fun(in, obj)
/* Increase the reference count for an object */
#define AKL_GC_INC_REF(obj) (obj)->gc_obj.gc_ref_count++
#define AKL_GC_SET_STATIC(obj) (obj)->gc_obj.gc_is_static = TRUE
/* Decrease the reference count for an object and free it
  if it's 'ref_count' is zero and if the object is not static. */
#define AKL_GC_DEC_REF(in, obj) if ((obj) && --(obj)->gc_obj.gc_ref_count == 0 \
                                && (!(obj)->gc_obj.gc_is_static)) \
                                AKL_GC_COLLECT_OBJ(in,obj)

void *akl_malloc(struct akl_state *, size_t);
struct akl_userdata {
    unsigned int ud_id; /* Exact user type */
    void *ud_private; /* Arbitrary userdata */
};

struct akl_lex_info {
    AKL_GC_DEFINE_OBJ;
    const char *li_name;
    unsigned int li_line; /* Line count */
    unsigned int li_count; /* Column count */
};

extern struct akl_value {
    AKL_GC_DEFINE_OBJ;
    struct akl_lex_info *va_lex_info;
    enum akl_type va_type;
    union {
        struct akl_atom *atom;
        char *string;
        double number;
        struct akl_list *list;
        akl_cfun_t cfunc;
        struct akl_userdata *udata;
    } va_value;

    bool_t is_quoted : 1;
    bool_t is_nil : 1;
} NIL_VALUE, TRUE_VALUE;

struct akl_atom {
    AKL_GC_DEFINE_OBJ;
    RB_ENTRY(akl_atom) at_entry;
    struct akl_value *at_value;
    char *at_name;
    char *at_desc; /* Documentation (mostly functions) */
    bool_t at_is_const : 1;  /* Is a constant atom? */
};

/* To properly handle both file and string sources, we
  must have to create an abstraction for the Lexical Analyzer*/
struct akl_io_device {
    device_type_t iod_type;
    union {
        FILE *file;
        const char *string;
    } iod_source;
    const char *iod_name; /* Name of the input device (for ex.: name of the file) */
    size_t iod_pos; /*  Only used for DEVICE_STRING */
    unsigned int iod_char_count;
    unsigned int iod_line_count;
    unsigned int iod_column;
};

#define AKL_NR_GC_STAT_ENT 6
enum { AKL_GC_STAT_ATOM,
       AKL_GC_STAT_LIST,
       AKL_GC_STAT_NUMBER,
       AKL_GC_STAT_STRING,
       AKL_GC_STAT_LIST_ENTRY,
       AKL_GC_STAT_ALLOC
};

struct akl_utype {
    const char *ut_name;
    akl_utype_t ut_id;
    akl_destructor_t ut_de_fun; /* Destructor for the given type */
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
#define AKL_LOAD_OK 0
#define AKL_LOAD_FAIL 1
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
#define AKL_VECTOR_NEW(type, count) akl_vector_new(sizeof(type), count)
struct akl_vector {
    void **av_vector;
    unsigned int av_count;
    unsigned int av_size;
};

struct akl_vector *akl_vector_new(struct akl_state *, unsigned int, unsigned int);
void akl_vector_push(akl_state *, struct akl_vector *, void *);
void *akl_vector_pop(akl_state *, struct akl_vector *);
void *akl_vector_at(akl_state *, struct akl_vector *);

struct akl_state {
    struct akl_io_device *ai_device;
    RB_HEAD(ATOM_TREE, akl_atom) ai_atom_head;
    unsigned int ai_gc_stat[AKL_NR_GC_STAT_ENT];
    struct akl_list *ai_program;
    /* Loaded user-defined types */
    struct akl_vector ai_utypes;
    /* Currently loaded modules */
    struct akl_vector ai_modules;
    struct akl_list *ai_errors; /* Collection of the errors (if any, default NULL) */
    bool_t ai_interactive;
};

static inline int cmp_atom(struct akl_atom *f, struct akl_atom *s)
{
    return strcasecmp(f->at_name, s->at_name);
}

RB_PROTOTYPE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

struct akl_atom *akl_add_global_atom(struct akl_state *, struct akl_atom *);
void akl_remove_global_atom(struct akl_state *, struct akl_atom *);
struct akl_atom *akl_add_builtin(struct akl_state *, const char *, akl_cfun_t, const char *);
struct akl_atom *akl_add_global_cfun(struct akl_state *, const char *, akl_cfun_t, const char *);
void akl_remove_function(struct akl_state *, akl_cfun_t);
struct akl_atom *akl_get_global_atom(struct akl_state *in, const char *);
void akl_do_on_all_atoms(struct akl_state *, void (*fn)(struct akl_atom *));
bool_t akl_is_equal_with(struct akl_atom *, const char **);

struct akl_list_entry {
    void *le_value;
    struct akl_list_entry *le_next;
};

struct akl_list {
    AKL_GC_DEFINE_OBJ;
    struct akl_list_entry *li_head;
    struct akl_list_entry *li_last; /* Last element (not the tail) */
    struct akl_list *li_parent; /* Parent (container) list */
    struct akl_atom **li_locals; /* Array of pointers to local variables */
    unsigned int li_elem_count;
    unsigned int li_local_count; /* Count of local variables pointed by *li_locals */
    bool_t is_quoted : 1;
/* Yep element count == 0 can simply mean NIL, but
  then we cannot use the AKL_IS_NIL() macro :-( */
    bool_t is_nil : 1;
};

#define AKL_LIST_FIRST(list) ((list)->li_head)
#define AKL_LIST_LAST(list) ((list)->li_last)
#define AKL_LIST_NEXT(ent) ((ent)->le_next)
#define AKL_LIST_SECOND(list) (AKL_LIST_NEXT(AKL_LIST_FIRST(list)))
#define AKL_FIRST_VALUE(list) akl_list_index_value(list, 0)
#define AKL_SECOND_VALUE(list) akl_list_index_value(list, 1)
#define AKL_THIRD_VALUE(list) akl_list_index_value(list, 2)
#define AKL_LIST_FOREACH(elem, list)  \
    for ((elem) = AKL_LIST_FIRST(list)\
       ; (elem)                        \
       ; (elem) = AKL_LIST_NEXT(elem))

/* Sometimes, useful to traverse the list from the second element */
#define AKL_LIST_FOREACH_SECOND(elem, list)  \
    for ((elem) = AKL_LIST_SECOND(list)\
       ; (elem)                        \
       ; (elem) = AKL_LIST_NEXT(elem))

/* If, the elem pointer will be modified (i.e: free()'d) you
  must use this macro */
#define AKL_LIST_FOREACH_SAFE(elem, list, tmp)  \
    for ((elem) = AKL_LIST_FIRST(list)          \
       ; (elem) && ((tmp) = AKL_LIST_NEXT(elem))  \
       ; (elem) = (tmp))

#define AKL_ENTRY_VALUE(elem) (struct akl_value *)((elem)->le_value)

typedef enum {
    tEOF,
    tNIL,
    tATOM,
    tNUMBER,
    tSTRING,
    tTRUE,
    tLBRACE, 
    tRBRACE,
    tQUOTE,
} token_t;



struct akl_state  *akl_new_file_interpreter(const char *, FILE *);
struct akl_state  *akl_new_string_interpreter(const char *, const char *);
struct akl_io_device *akl_new_file_device(const char *, FILE *);
struct akl_io_device *akl_new_string_device(const char *, const char *);
struct akl_state  *
akl_reset_string_interpreter(struct akl_state *in, const char *name, const char *str);

token_t akl_lex(struct akl_io_device *);
void    akl_lex_free(void);
char   *akl_lex_get_string(void);
double  akl_lex_get_number(void);
char   *akl_lex_get_atom(void);

struct akl_list  *akl_parse_list(struct akl_state *, struct akl_io_device *);
struct akl_value *akl_parse_value(struct akl_state *, struct akl_io_device *);
struct akl_list  *akl_parse_file(struct akl_state *, FILE *);
struct akl_list  *akl_parse_string(struct akl_state *, const char *);
struct akl_list  *akl_parse_io(struct akl_state *, struct akl_io_device *);
struct akl_list  *akl_parse(struct akl_state *);
struct akl_value *akl_car(struct akl_list *);
struct akl_list  *akl_cdr(struct akl_state *, struct akl_list *);

/* Creating and destroying structures */
struct akl_state   *akl_new_state(void);
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

char                  *akl_num_to_str(struct akl_state *, double);
struct akl_value      *akl_to_number(struct akl_state *, struct akl_value *);
struct akl_value      *akl_to_string(struct akl_state *, struct akl_value *);
struct akl_value      *akl_to_atom(struct akl_state *, struct akl_value *);
struct akl_value      *akl_to_symbol(struct akl_state *, struct akl_value *);

char *akl_get_atom_name_value(struct akl_value *);
akl_utype_t akl_get_utype_value(struct akl_value *);
void *akl_get_udata_value(struct akl_value *);
struct akl_userdata *akl_get_userdata_value(struct akl_value *);
bool_t akl_check_user_type(struct akl_value *, akl_utype_t);
struct akl_module *akl_get_module_descriptor(struct akl_state *, struct akl_value *);

void akl_free_atom(struct akl_state *in, struct akl_atom *atom);
void akl_free_value(struct akl_state *in, struct akl_value *val);
void akl_free_list_entry(struct akl_state *in, struct akl_list_entry *ent);
void akl_free_list(struct akl_state *in, struct akl_list *list);
void akl_free_state(struct akl_state *in);

struct akl_list_entry *
akl_list_append(struct akl_state *, struct akl_list *, void *);
struct akl_list_entry *
akl_list_append_value(struct akl_state *, struct akl_list *, struct akl_value *);

struct akl_list_entry *
akl_list_insert_head(struct akl_state *, struct akl_list *, void *);
struct akl_list_entry *
akl_list_insert_value_head(struct akl_state *, struct akl_list *, struct akl_value *);

struct akl_value *akl_list_index_value(struct akl_list *, int);
struct akl_list_entry *akl_list_find(struct akl_list *, struct akl_value *);
struct akl_value *akl_duplicate_value(struct akl_state *, struct akl_value *);
struct akl_list *akl_list_duplicate(struct akl_state *, struct akl_list *);

int    akl_io_getc(struct akl_io_device *);
int    akl_io_ungetc(int, struct akl_io_device *);
bool_t akl_io_eof(struct akl_io_device *dev);

struct akl_value *akl_eval_value(struct akl_state *, struct akl_value *);
struct akl_value *akl_eval_list(struct akl_state *, struct akl_list *);
void akl_eval_program(struct akl_state *);

void akl_print_value(struct akl_state *, struct akl_value *);
void akl_print_list(struct akl_state *, struct akl_list *);
int akl_compare_values(struct akl_value *, struct akl_value *);
int akl_get_typeid(struct akl_state *, const char *);

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
unsigned int akl_register_type(struct akl_state *, const char *, akl_destructor_t);
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
