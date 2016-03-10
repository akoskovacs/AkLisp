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

#ifndef AKL_MALLOC
# define AKL_MALLOC(s, type) (type *)akl_alloc(s, sizeof(type))
#endif // AKL_MALLOC

#ifndef AKL_FREE
# define AKL_FREE(s, obj) akl_free(s, (void *)obj, sizeof *obj)
#endif // AKL_FREE

#ifndef AKL_STRDUP
# define AKL_STRDUP(str) strdup(str)
#endif // AKL_STRDUP

#define AKL_CHECK_TYPE(v1, type) (((v1) && (v1)->va_type == (type)) ? TRUE : FALSE)
#define AKL_TYPE(value) (value->va_type)
#define AKL_GET_VALUE_MEMBER_PTR(val, type, member) \
                            ((AKL_CHECK_TYPE(val, type) \
                            ? (val)->va_value.member : NULL))

#define AKL_GET_VALUE_MEMBER(val, type, member) \
                            ((AKL_CHECK_TYPE(val, type) \
                            ? (val)->va_value.member : 0))

#define AKL_GET_NUMBER_VALUE(val) (AKL_GET_VALUE_MEMBER(val, AKL_VT_NUMBER, number))
#define AKL_GET_STRING_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, AKL_VT_STRING, string))
#define AKL_GET_LIST_VALUE(val) (AKL_GET_VALUE_MEMBER_PTR(val, AKL_VT_LIST, list))
#define AKL_STACK_SIZE 32
#define AKL_SET_FEATURE(state, feature) ((state)->ai_config |= (feature))
#define AKL_UNSET_FEATURE(state, feature) ((state)->ai_config &= ~(feature))
#define AKL_IS_FEATURE_ON(state, feature) ((state) && ((state)->ai_config & (feature)))
#define AKL_NOTHING  /* empty for assert */
#define AKL_ASSERT(expr, retval) \
    assert(expr);                \
    if (!(expr)) {               \
        return retval; }
/*
 * Make constant string parameter passing easy.
 * Mostly for functions with 'char *str, bool_t is_cstr'
 * style arguments.
 * XXX: Used only in function calls.
*/
#define AKL_CSTR(str) (char *)str,TRUE
/* Same as above, but only for mutable strings. */
#define AKL_MSTR(str) (char *)str,FALSE
/* Converts NULL to AKL_NIL */
#define AKL_NULLER(expr) (((expr) == NULL) ? AKL_NIL : expr)

struct akl_list;
struct akl_value;
struct akl_io_device;
struct akl_state;
struct akl_vector;
struct akl_function;
struct akl_context;

enum AKL_VALUE_TYPE {
    AKL_VT_NIL,
    AKL_VT_SYMBOL,
    AKL_VT_VARIABLE,
    AKL_VT_NUMBER,
    AKL_VT_STRING,
    AKL_VT_LIST,
    AKL_VT_TRUE,
    AKL_VT_FUNCTION,
    AKL_VT_USERDATA
};

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

const char *akl_type_name[10];

typedef enum { FALSE, TRUE } bool_t;
typedef enum { DEVICE_FILE, DEVICE_STRING } device_type_t;
/* Ordinary C functions a.k.a. normal S-expressions*/
typedef struct akl_value*(*akl_cfun_t)(struct akl_context *, int);
/* Special C functions a.k.a. special forms */
typedef struct akl_function *(*akl_sfun_t)(struct akl_context *);
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
 * for the mark-and-sweep garbage collector.
 */
/* Size of the GC pools, must be the power of 2 */
#define AKL_GC_POOL_SIZE 64
#define AKL_GC_DEFINE_OBJ       struct akl_gc_object gc_obj
#define AKL_GC_SET_MARK(obj, m) ((obj)->gc_obj.gc_mark = m)
#define AKL_GC_SET_VALUE_LIST(obj) ((obj)->gc_obj.gc_le_is_obj = TRUE)
#define AKL_GC_TYPE_ID(obj)     ((obj)->gc_obj.gc_type_id)
#define AKL_GC_IS_MARKED(obj)   ((obj)->gc_obj.gc_mark == TRUE)
#define AKL_GC_SET_STATIC(obj)  ((obj)->gc_obj.gc_static = TRUE)
#define AKL_GC_INIT_OBJ(obj, id) \
    (obj)->gc_obj.gc_type_id = id; (obj)->gc_obj.gc_static = FALSE; \
    (obj)->gc_obj.gc_mark = FALSE ; (obj)->gc_obj.gc_le_is_obj = FALSE;

/* GC'd object's finalizer */
typedef void (*akl_gc_destructor_t)(struct akl_state *, void *obj);
/* Mark and unmark GC'd objects */
typedef void (*akl_gc_marker_t)(struct akl_state *, void *, bool_t);
/*
 * Every structure, which is suitable for collection, have to
 * embed this structure as the first variable, with the name of 'gc_obj'.
*/
typedef unsigned int akl_gc_type_t;
struct akl_gc_object {
    akl_gc_type_t       gc_type_id;
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
    struct akl_list       *li_parent; /* Parent (container) list */
    unsigned int           li_count;
    bool_t                 is_quoted : 1;
/* Yep element count == 0 can simply mean NIL, but
  then we cannot use the AKL_IS_NIL() macro :-( */
    bool_t                 is_nil    : 1;
};

struct akl_userdata {
    unsigned int ud_id;      /* Exact user type identifer */
    void        *ud_private; /* Arbitrary userdata */
};

struct akl_lex_info {
    const char  *li_name;
    unsigned int li_line;  /* Line count */
    unsigned int li_count; /* Column count */
};

extern struct akl_value {
    AKL_GC_DEFINE_OBJ;
    struct akl_lex_info     *va_lex_info;
    enum AKL_VALUE_TYPE      va_type;
    union {
        double               number;
        struct akl_symbol   *symbol;
        char                *string;
        struct akl_function *func;
        struct akl_userdata *udata;
		struct akl_list     *list;
    } va_value;

    bool_t                   is_quoted : 1;
    bool_t                   is_nil    : 1;
} NIL_VALUE, TRUE_VALUE;
#define AKL_NIL &NIL_VALUE

struct akl_symbol {
    char                   *sb_name;
    /* Symbols only exist in one instance and
     * only in the symbol tree.
     * You can simply compare two symbols by their pointers.
     */
    RB_ENTRY(akl_symbol)    sb_entry;
    /* We must know the the constness of the name. */
    bool_t                  sb_is_cdef : 1;
};

struct akl_symbol *akl_new_symbol(struct akl_state *, char *, bool_t);
struct akl_symbol *akl_get_symbol(struct akl_state *s, char *name);
struct akl_symbol *akl_get_or_create_symbol(struct akl_state *s, char *name);

struct akl_variable {
    AKL_GC_DEFINE_OBJ;
    struct akl_symbol      *vr_symbol;        /* Name (as a symbol in the symbol tree) */
    struct akl_value       *vr_value;         /* Associated value */
    struct akl_lex_info    *vr_lex_info;      /* Lexical information (definition position) */
    RB_ENTRY(akl_variable)  vr_entry;         /* Red-Black tree entry in the global variable tree */
    char                   *vr_desc;          /* Documentation (mostly functions) */
    bool_t                  vr_is_cdesc : 1;  /* True when vr_desc is const char */
    bool_t                  vr_is_const : 1;  /* True on immutable "variables" */
};

/* To properly handle both file and string sources, we
  must have to create an abstraction for the Lexical Analyzer*/
struct akl_io_device {
    device_type_t     iod_type;
    union {
        FILE         *file;
        const char   *string;
    } iod_source;

    const char       *iod_name;    /* Name of the input device (for ex.: name of the file) */
    size_t            iod_pos;     /*  Only used for DEVICE_STRING */
    char             *iod_buffer;  /* Lexer buffer */
    struct akl_state *iod_state;
    unsigned int      iod_buffer_size;
    unsigned int      iod_char_count;
    unsigned int      iod_line_count;
    unsigned int      iod_column;
    akl_token_t       iod_backlog; /* akl_lex_putback() will put the token to here */
};

/* Stack frame information */
struct akl_frame {
    struct akl_list_entry *ent;
};

void akl_init_frame(struct akl_context *, int argc);

/* Contextual environment */
struct akl_context {
    /* Current context for different entities */
    struct akl_state        *cx_state;     /* Current state */
    struct akl_list         *cx_stack;     /* Pointer to the current stack (probably '&cx_state->ai_stack') */
    struct akl_list         *cx_ir;        /* The current Internal Representation */
    struct akl_function     *cx_func;      /* The called function's descriptor */
    struct akl_context      *cx_parent;    /* Parent context pointer */
    struct akl_list         *cx_frame;     /* Frame info used by executor (push) */
    unsigned int             cx_frame_len; /* Length of the frame */

    const char           *cx_func_name; /* The called function's name */
    struct akl_function  *cx_comp_func; /* The function under compilation */
    struct akl_io_device *cx_dev;       /* The current I/O device */
    struct akl_lex_info  *cx_lex_info;  /* Current lexical information */
    struct akl_function  *cx_fn_main;   /* The main function */
};

struct akl_context *akl_new_context(struct akl_state *);
void akl_init_context(struct akl_context *);
void akl_execute_ir(struct akl_context *);
void akl_execute(struct akl_context *);
void akl_dump_ir(struct akl_context *, struct akl_function *);
void akl_clear_ir(struct akl_context *);
void akl_dump_stack(struct akl_context *);

/* User-definied value type */
struct akl_utype {
    const char         *ut_name;   /* Name of this utype */
    akl_utype_t         ut_id;     /* ID of this utype */
    akl_gc_destructor_t ut_de_fun; /* Destructor function, called by the GC on finialization */
};

enum AKL_FUNCTION_TYPE {
    AKL_FUNC_CFUN,    /* Normal built-in C functions */
    AKL_FUNC_SPECIAL, /* Compiler, optimizing control structures, etc... (if, while, ...) */
    AKL_FUNC_NOEVAL,  /* Built-ins, without argument 'pre-evaulation' (set!, ...) */
    AKL_FUNC_USER,    /* User-defined Lisp functions */
    AKL_FUNC_LAMBDA   /* User-defined lambdas */
};

/* Used only for declaring functions implemented in C */
struct akl_fun_decl {
    enum AKL_FUNCTION_TYPE fd_type;
    union {
        akl_cfun_t cfun;
        akl_sfun_t sfun;
    } fd_fun;
    const char *fd_name;
    const char *fd_desc;
};

typedef int (*akl_mod_load_t)(struct akl_state *);
struct akl_module {
    const char          *am_path;       /* The absolute path of the module binary */
    const char          *am_name;       /* Module name */
    const char          *am_desc;       /* Text description of the module */
    const char          *am_author;     /* Author's name and email, like 'Prog C. Rammer <progc@somewhe.re>' */
    struct akl_fun_decl *am_funs;       /* Exported functions */
    size_t               am_size;       /* Size of the module */
    const char         **am_depends_on; /* Names of the required modules (NULL terminated) */
    void                *am_handle;     /* dlopen()'s handle */
    akl_mod_load_t       am_load;       /* Loader function */
    akl_mod_load_t       am_unload;     /* Unloader function */
};
enum AKL_MOD_STATUS { AKL_LOAD_FAIL = 0, AKL_LOAD_OK  };
/* The first three arguments cannot be NULL */
#define AKL_MODULE_DEFINE(funs, load, unload, name, desc, author) \
struct akl_module __module_desc = { \
    .am_funs   = funs,   \
    .am_name   = name,   \
    .am_path   = NULL,   \
    .am_handle = NULL    \
    .am_author = author, \
    .am_size   = 0,      \
    .am_desc   = desc,   \
    .am_load   = load,   \
    .am_unload = unload  \
}

#define AKL_MODULE const static struct akl_module __mod_desc = 
#define AKL_MOD_NAME(name)        .am_name  = (name)
#define AKL_MOD_HELP(desc)        .am_desc  = (desc)
#define AKL_MOD_DESCRIPTION(desc) .am_desc  = (desc)
#define AKL_MOD_AUTHOR(name)      .am_author = (name)

#define AKL_VECTOR_DEFSIZE 10
#define AKL_VECTOR_NEW(s, type, count) akl_vector_new(s, sizeof(type), count)
#define AKL_VECTOR_FOREACH(ind, ptr, vec)            \
    for ( (ind) = 0, (ptr) = akl_vector_at(vec, 0)   \
        ; (ind) < akl_vector_count(vec)              \
        ; (ind)++, (ptr) = akl_vector_at(vec, ind))

/* Foreach on all elements including the NULL ones */
#define AKL_VECTOR_FOREACH_ALL(ind, ptr, vec) \
    for ((ind) = 0, (ptr) = akl_vector_at(vec, 0) \
        ;(ind) < akl_vector_size(vec); (ind)++, (ptr) = akl_vector_at(vec, ind))

struct akl_vector {
    char             *av_vector; /* Array of objects */
    unsigned int      av_count;  /* Count of the added objects */
    unsigned int      av_size;   /* Size of the array */
    unsigned int      av_msize;  /* Size of the members in the array */
    /* Full_size = av_size * av_msize; see calloc() */
    struct akl_state *av_state; /* Need for memory management */
};

unsigned int akl_vector_size(struct akl_vector *);
unsigned int akl_vector_count(struct akl_vector *);
struct akl_vector *
akl_new_vector(struct akl_state *, unsigned int, unsigned int);
void         akl_init_vector(struct akl_state *, struct akl_vector *, unsigned int, unsigned int);
void        *akl_vector_reserve(struct akl_vector *);
void        *akl_vector_reserve_more(struct akl_vector *, int);
//unsigned int akl_vector_add(struct akl_vector *, void *);
unsigned int akl_vector_push(struct akl_vector *, void *);
bool_t		 akl_vector_is_empty(struct akl_vector *);
bool_t       akl_vector_is_grow_need(struct akl_vector *);
void        *akl_vector_remove(struct akl_vector *, unsigned int);
void        *akl_vector_pop(struct akl_vector *);
void        *akl_vector_find(struct akl_vector *, akl_cmp_fn_t, void *, int *);
void         akl_vector_push_vec(struct akl_vector *, struct akl_vector *);
void        *akl_vector_at(struct akl_vector *, unsigned int);
void         akl_vector_set(struct akl_vector *, unsigned int, void *);
void        *akl_vector_last(struct akl_vector *);
void        *akl_vector_first(struct akl_vector *);
bool_t       akl_vector_is_empty_at(struct akl_vector *, unsigned int);
void         akl_vector_destroy(struct akl_state *, struct akl_vector *);
void         akl_vector_free(struct akl_state *, struct akl_vector *);
void         akl_vector_grow(struct akl_vector *, unsigned int);
void         akl_vector_truncate_by(struct akl_vector *, unsigned int);

struct akl_lisp_fun {
    /* Name of the arguments */
    //char               **uf_args;
    struct akl_vector    uf_args;
    /* Name of the local variables */
    /* TODO: struct akl_vector   uf_locals; */
    /* List of the instructions,
        (li_parent is the full IR) */
    struct akl_list      uf_body;
    /* Start of the function */
    struct akl_list      uf_labels;
    struct akl_lex_info *uf_info;
};

struct akl_function {
    AKL_GC_DEFINE_OBJ;
    enum AKL_FUNCTION_TYPE fn_type;
    /* Body of the function */
    union {
        /* Bytecode lisp function */
        struct akl_lisp_fun    ufun;
        /* C function (normal form) */
        akl_cfun_t             cfun;
        /* C function (special form) */
        akl_sfun_t             scfun;
    } fn_body;
    /* Argument count */
#define AKL_ARG_OPTIONAL -1
#define AKL_ARG_REST     -2
};

struct akl_function *
akl_call_sform(struct akl_context *, struct akl_symbol *, struct akl_function *);
struct akl_value *
akl_call_function_bound(struct akl_context *, int);
/* The second argument is the function's own local context. */
struct akl_value *
akl_call_symbol(struct akl_context *, struct akl_context *, struct akl_symbol *, int);
struct akl_value *
akl_call_function(struct akl_context *, struct akl_context *, const char *, int);
struct akl_context *
akl_bound_function(struct akl_context *, struct akl_symbol *, struct akl_function *);

struct akl_gc_pool {
    struct akl_vector    gp_pool;
    struct akl_gc_pool  *gp_next;
    /* Bitmap of the free slots */
    unsigned int         gp_freemap[AKL_GC_POOL_SIZE/(sizeof(unsigned int)*8)];
};

struct akl_gc_type {
    akl_gc_type_t       gt_type_id;
    size_t              gt_type_size;
    akl_gc_marker_t     gt_marker_fn;

    struct akl_gc_pool *gt_pool_last;
    unsigned int        gt_pool_count;
    struct akl_gc_pool *gt_pool_head;
};

/*
 * Usage:
 * AKL_DEFINE_FUNCTION(testfn, ctx, argc) {
 *   struct akl_value *a1, *a2, *a3;
 *   akl_get_value_args(ctx, 3, &a1, &a2, &a3);
 * ...
 * Returns -1 on error
*/
int akl_get_args(struct akl_context *, int, ...);
/*
 * Usage:
 * AKL_DEFINE_FUNCTION(testfn, ctx, argc) {
 *   struct akl_function *f;
 *   double num;
 *   bool_t is_nil;
 *   akl_get_value_args(ctx, 3, TYPE_NIL, &is_nil, TYPE_FUNCTION, &f, TYPE_NUMBER, &num);
 * ...
 * Returns -1 on error
*/
int akl_get_args_strict(struct akl_context *, int argc, ...);
struct akl_value *akl_frame_pop(struct akl_context *);
double *akl_frame_pop_number(struct akl_context *);
double *akl_frame_shift_number(struct akl_context *);
char   *akl_frame_pop_string(struct akl_context *);
char   *akl_frame_shift_string(struct akl_context *);
struct akl_list *akl_frame_pop_list(struct akl_context *);
struct akl_list *akl_frame_shift_list(struct akl_context *);
struct akl_value *akl_frame_top(struct akl_context *);
enum   AKL_VALUE_TYPE akl_stack_top_type(struct akl_context *);
struct akl_value *akl_frame_top(struct akl_context *);
struct akl_value *akl_frame_shift(struct akl_context *);
struct akl_value *akl_frame_head(struct akl_context *);

void   akl_stack_push(struct akl_context *, struct akl_value *);
void   akl_frame_push(struct akl_context *, struct akl_value *);
struct akl_value *akl_stack_pop(struct akl_context *);

typedef enum {
    AKL_NM_TERMINATE, AKL_NM_TRYAGAIN, AKL_NM_RETNULL
} akl_nomem_action_t;

akl_nomem_action_t akl_def_nomem_handler(struct akl_state *);

#define AKL_GC_NR_BASE_TYPES 6
typedef enum {
       AKL_GC_VALUE = 0,
       AKL_GC_VARIABLE,
       AKL_GC_LIST,
       AKL_GC_LIST_ENTRY,
       AKL_GC_FUNCTION,
       AKL_GC_UDATA
} akl_gc_base_type_t;

struct akl_mem_callbacks {
    void *(*mc_malloc_fn) (size_t);
    void *(*mc_calloc_fn) (size_t, size_t);
    void  (*mc_free_fn)   (void *);
    void *(*mc_realloc_fn)(void *, size_t);
    akl_nomem_action_t (*mc_nomem_fn)(struct akl_state *);
};

struct akl_mem_callbacks akl_mem_std_callbacks;
void   akl_set_mem_callbacks(struct akl_state *, const struct akl_mem_callbacks *);

/* An instance of the interpreter */
struct akl_state {
    const struct akl_mem_callbacks *ai_mem_fn;
    struct akl_io_device           *ai_device;
    RB_HEAD(SYM_TREE, akl_symbol)   ai_symbols;
    RB_HEAD(VAR_TREE, akl_variable) ai_global_vars;
    unsigned int                    ai_gc_malloc_size; /* Totally malloc()'d bytes */
    struct akl_vector               ai_gc_types;

    /* Loaded user-defined types */
    struct akl_vector               ai_utypes;
    /* Currently loaded modules */
    struct akl_list                 ai_modules;
    struct akl_context              ai_context;   /* The main context  */
    struct akl_list                 ai_stack;     /* The main stack list */
    struct akl_list                *ai_errors;    /* Collection of the errors (if any, default NULL) */
    #define AKL_CFG_USE_COLORS      0x0001
    #define AKL_CFG_USE_GC          0x0002
    #define AKL_CFG_INTERACTIVE     0x0004               /* Interactive interpreter */
    #define AKL_DEBUG_INSTR         0x0008
    #define AKL_DEBUG_STACK         0x0010
    unsigned long                   ai_config; /* Bit configuration */
    bool_t                          ai_gc_last_was_mark : 1;
    bool_t                          ai_interrupted :1;  /* The program is stopped by an interrupt  */
};

bool_t akl_set_feature(struct akl_state *, const char *);
bool_t akl_set_feature_to(struct akl_state *, const char *, bool_t);

struct akl_label *akl_new_branches(struct akl_state *, struct akl_context *);
struct akl_list  *akl_new_labels(struct akl_context *, int *, int);
struct akl_label *akl_new_label(struct akl_context *);

struct akl_label {
    struct akl_list       *la_ir;
    struct akl_list_entry *la_branch;
    unsigned               la_ind;
    char                  *la_name; // Only used when assembling
};

typedef enum {
    AKL_IR_NOP = 0,
    AKL_IR_PUSH,
    AKL_IR_LOAD,
    AKL_IR_CALL,
    AKL_IR_GET,
    AKL_IR_SET,
    AKL_IR_BRANCH,
    AKL_IR_JMP,    /* Unconditional jump */
    AKL_IR_JT,     /* Jump if true */
    AKL_IR_JN,     /* Jump if false, (not true, nil) */
    AKL_IR_HEAD,
    AKL_IR_TAIL,
    AKL_IR_RET
} akl_ir_instruction_t;

#define AKL_NR_INSTRUCTIONS 14
const char *akl_ir_instruction_set[AKL_NR_INSTRUCTIONS];

typedef enum {
    AKL_JMP       = AKL_IR_JMP,
    AKL_JMP_TRUE  = AKL_IR_JT,
    AKL_JMP_FALSE = AKL_IR_JN
} akl_jump_t;

struct akl_ir_instruction {
    akl_ir_instruction_t     in_op;  /* Operation */
    /* Optional (used if the function already resolved) */
    struct akl_function     *in_fun;
    union {
        struct akl_value    *value;  /* Generic value (mostly used by push) */
        struct akl_symbol   *symbol; /* Name of the variable of function    */
        struct akl_label    *label;  /* Label for the next instruction      */
        unsigned int         ui_num; /* Stack pointer or argument count     */
    } in_arg[2];
    struct akl_lex_info     *in_linfo; /* Lexical information of this instruction */
};

struct akl_function *akl_compile_list(struct akl_context *);
void akl_build_branch(struct akl_context *, struct akl_list *, int, int);
void akl_build_jump(struct akl_context *, akl_jump_t, struct akl_list *, int);
/* Call by symbol or function */
void akl_build_call(struct akl_context *, struct akl_symbol *, struct akl_function *, int);
void akl_build_label(struct akl_context *, struct akl_list *, int);
void akl_build_set(struct akl_context *, struct akl_symbol *);
void akl_build_get(struct akl_context *, struct akl_symbol *);
void akl_build_load(struct akl_context *, struct akl_symbol *);
void akl_build_push(struct akl_context *, struct akl_value *);
void akl_build_nop(struct akl_context *);
void akl_build_ret(struct akl_context *);
/* Helper functions for the Red-Black trees */

/* Order symbols by name.
 * XXX: Used by the tree builder functions.
*/
static inline int
akl_rb_cmp_sym(struct akl_symbol *f, struct akl_symbol *s)
{
    AKL_ASSERT(f && s && f->sb_name && s->sb_name, 1);
    return strcasecmp(f->sb_name, s->sb_name);
}

static inline int
akl_rb_cmp_var(struct akl_variable *f, struct akl_variable *s)
{
    AKL_ASSERT(f && s && f->vr_symbol && s->vr_symbol, 1);
    return akl_rb_cmp_sym(f->vr_symbol, s->vr_symbol);
}

/* Generate prototypes for the Red-Black trees */
RB_PROTOTYPE(VAR_TREE, akl_variable, vr_entry, akl_rb_cmp_var);
RB_PROTOTYPE(SYM_TREE, akl_symbol,   sb_entry, akl_rb_cmp_sym);

struct akl_variable *akl_set_global_var(struct akl_state *s, struct akl_symbol *
                        , char *desc, bool_t is_cdesc
                        , struct akl_value *v);
struct akl_variable *akl_set_global_variable(struct akl_state *s
                        , char *name, bool_t is_cname
                        , char *desc, bool_t is_cdesc
                        , struct akl_value *v);
void   akl_add_global_cfun(struct akl_state *, akl_cfun_t, const char *, const char *);
void   akl_add_global_sfun(struct akl_state *, akl_sfun_t, const char *, const char *);
void   akl_remove_function(struct akl_state *, akl_cfun_t);
struct akl_variable *
akl_get_global_variable(struct akl_state *, char *);
struct akl_value *
akl_get_global_value(struct akl_state *, char *);
struct akl_variable *
akl_get_global_var(struct akl_state *, struct akl_symbol *sym);
void   akl_do_on_all_vars(struct akl_state *, void (*)(struct akl_variable *));
void   akl_do_on_all_syms(struct akl_state *, void (*)(struct akl_symbol *));
bool_t akl_is_strings_include(struct akl_symbol *, const char **);
bool_t akl_var_is_function(struct akl_variable *);
struct akl_function *
akl_var_to_function(struct akl_variable *);

#define AKL_LIST_FIRST(list) ((list != NULL) ? (list)->li_head : NULL)
#define AKL_LIST_LAST(list) ((list != NULL) ? (list)->li_last : NULL)
#define AKL_LIST_NEXT(ent) ((ent)->le_next)
#define AKL_LIST_PREV(ent) ((ent)->le_prev)
#define AKL_LIST_SECOND(list) (AKL_LIST_NEXT(AKL_LIST_FIRST(list)))
#define AKL_FIRST_VALUE(list) akl_list_index_value(list, 0)
#define AKL_SECOND_VALUE(list) akl_list_index_value(list, 1)
#define AKL_THIRD_VALUE(list) akl_list_value(list, 2)
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
    for ((elem) = (list)->li_head \
       ; (elem)                        \
       ; (elem) = AKL_LIST_NEXT(elem))

#define AKL_LIST_FOREACH_BACK(elem, list) \
    for ((elem) = (list)->li_last     \
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

/* A 'strict' foreach. It stops on the last element */
#define AKL_LIST_FOREACH_STRICT(elem, list)      \
    for ((elem) = AKL_LIST_FIRST(list)           \
       ; (elem) && (elem) != AKL_LIST_LAST(list) \
       ; (elem) = AKL_LIST_NEXT(elem))

#define AKL_ENTRY_VALUE(elem) (struct akl_value *)((elem) ? (elem)->le_data : NULL)

typedef enum {
    tASM_EOF,
    tASM_WORD=tATOM,
    tASM_STRING=tSTRING,
    tASM_NUMBER=tNUMBER,
    tASM_DOT='.',
    tASM_COMMA=',',
    tASM_COLON=':',
    tASM_PERC='%',
    tASM_FDECL='@'
} akl_asm_token_t;

void *akl_alloc(struct akl_state *, size_t);
void *akl_calloc(struct akl_state *, size_t, size_t);
void *akl_realloc(struct akl_state *, void *, size_t);
/* The third parameter of akl_free() is not mandatory. */
void akl_free(struct akl_state *, void *, size_t);

struct akl_state     *akl_new_file_interpreter(const char *, FILE *, const struct akl_mem_callbacks *);
struct akl_state     *akl_new_string_interpreter(const char *, const char *, const struct akl_mem_callbacks *);
struct akl_io_device *akl_new_file_device(struct akl_state *, const char *, FILE *);
struct akl_io_device *akl_new_string_device(struct akl_state *, const char *, const char *);
struct akl_state     *akl_reset_string_interpreter(struct akl_state *, const char *, const char *
                                                   , const struct akl_mem_callbacks *);
void                  akl_init_string_device(const char *name, const char *str);
void                  akl_init_string_device(const char *name, const char *str);

akl_token_t akl_lex(struct akl_io_device *);
void akl_lex_putback(struct akl_io_device *, akl_token_t);
akl_asm_token_t akl_asm_lex(struct akl_io_device *);
void    akl_lex_free(struct akl_io_device *);
char   *akl_lex_get_string(struct akl_io_device *);
double  akl_lex_get_number(struct akl_io_device *);
struct akl_symbol *akl_lex_get_symbol(struct akl_io_device *);

akl_token_t akl_compile_next(struct akl_context *, struct akl_function **fn);
struct akl_value *
akl_parse_token(struct akl_context *, akl_token_t, bool_t);
struct akl_list  *akl_parse_list(struct akl_context *);
struct akl_list *akl_str_to_list(struct akl_state *, const char *);
struct akl_value *akl_build_value(struct akl_context *, akl_token_t);
struct akl_value *akl_parse_value(struct akl_context *);
struct akl_list  *akl_parse_file(struct akl_state *, const char *, FILE *);
struct akl_list  *akl_parse_io(struct akl_state *, struct akl_io_device *);
struct akl_list  *akl_parse(struct akl_state *);
struct akl_value *akl_car(struct akl_list *);
struct akl_list  *akl_cdr(struct akl_state *, struct akl_list *);
struct akl_list  *akl_list_tail(struct akl_state *, struct akl_list *);
struct akl_list_entry *akl_list_it_begin(struct akl_list *);
struct akl_list_entry *akl_list_it_end(struct akl_list *);
bool_t akl_list_it_has_next(struct akl_list_entry *);
bool_t akl_list_it_has_prev(struct akl_list_entry *);
void  *akl_list_it_next(struct akl_list_entry **);
void  *akl_list_it_prev(struct akl_list_entry **);

void akl_asm_parse_instr(struct akl_context *);
void akl_asm_parse_decl(struct akl_context *);

/* Creating and destroying structures */
/* If the memory callbacks are NULL, the standard allocation functions will be used */
void                   akl_init_state(struct akl_state *, const struct akl_mem_callbacks *);
struct akl_state      *akl_new_state(const struct akl_mem_callbacks *);
struct akl_function   *akl_new_function(struct akl_state *);
struct akl_value      *akl_new_function_value(struct akl_state *, struct akl_function *);
void                   akl_init_list(struct akl_list *);
struct akl_list       *akl_new_list(struct akl_state *);
struct akl_variable   *akl_new_variable(struct akl_state *, char *, bool_t);
struct akl_variable   *akl_new_var(struct akl_state *, struct akl_symbol *);
struct akl_list_entry *akl_new_list_entry(struct akl_state *);
struct akl_value      *akl_new_value(struct akl_state *);
struct akl_value      *akl_new_nil_value(struct akl_state *s);
struct akl_value      *akl_new_true_value(struct akl_state *s);
struct akl_value      *akl_new_string_value(struct akl_state *, char *);
struct akl_value      *akl_new_number_value(struct akl_state *, double);
struct akl_value      *akl_new_list_value(struct akl_state *, struct akl_list *);
struct akl_value      *akl_new_symbol_value(struct akl_state *, char *, bool_t);
struct akl_value      *akl_new_sym_value(struct akl_state *, struct akl_symbol *);
struct akl_value      *akl_new_variable_value(struct akl_state *, char *, bool_t);
struct akl_value      *akl_new_user_value(struct akl_state *, akl_utype_t, void *);
struct akl_lex_info   *akl_new_lex_info(struct akl_state *, struct akl_io_device *);

/* Aliases for value creation (with just the context) */
#define AKL_NUMBER(ctx, num) ((ctx && ctx->cx_state) ? akl_new_number_value(ctx->cx_state, num) : NULL)
#define AKL_STRING(ctx, str) ((ctx && ctx->cx_state) ? akl_new_string_value(ctx->cx_state, str) : NULL)
#define AKL_ATOM(ctx, aname) ((ctx && ctx->cx_state) ? akl_new_atom_value(ctx->cx_state, aname) : NULL)
#define AKL_LIST(ctx, l)     ((ctx && ctx->cx_state) ? akl_new_list_value(ctx->cx_state, l)     : NULL)

/* GC functions */
void   akl_gc_init(struct akl_state *);
akl_gc_type_t akl_gc_register_type(struct akl_state *, akl_gc_marker_t, size_t);
//void akl_gc_deregister_type(struct akl_state *, akl_gc_type_t);
struct akl_gc_type *akl_gc_get_type(struct akl_state *, akl_gc_type_t);

void   akl_gc_mark(struct akl_state *);
void   akl_gc_mark_object(struct akl_state *, void *, bool_t);
void   akl_gc_sweep_pool(struct akl_state *, struct akl_gc_pool *, akl_gc_marker_t);
void   akl_gc_sweep(struct akl_state *);
void   akl_gc_enable(struct akl_state *);
void   akl_gc_disable(struct akl_state *);
struct akl_gc_pool *akl_gc_pool_create(struct akl_state *, struct akl_gc_type *);
bool_t akl_gc_pool_is_empty(struct akl_gc_pool *);
bool_t akl_gc_tryfree(struct akl_state *);
void   akl_gc_pool_free(struct akl_state *, struct akl_gc_pool *);
void  *akl_gc_malloc(struct akl_state *, akl_gc_type_t);

char             *akl_num_to_str(struct akl_state *, double);
struct akl_value *akl_to_number(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_string(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_atom(struct akl_state *, struct akl_value *);
struct akl_value *akl_to_symbol(struct akl_state *, struct akl_value *);

akl_utype_t akl_get_utype_value(struct akl_value *);
void       *akl_get_udata_value(struct akl_value *);
struct      akl_userdata *akl_get_userdata_value(struct akl_value *);
bool_t      akl_check_user_type(struct akl_value *, akl_utype_t);
struct akl_module *akl_get_module_descriptor(struct akl_state *, struct akl_value *);

struct akl_list_entry *
    akl_list_append(struct akl_state *, struct akl_list *, void *);
struct akl_list_entry *
    akl_list_append_value(struct akl_state *, struct akl_list *, struct akl_value *);

struct akl_list_entry *
    akl_list_insert_head(struct akl_state *, struct akl_list *, void *);
struct akl_list_entry *
    akl_list_insert_head_value(struct akl_state *, struct akl_list *, struct akl_value *);
void *akl_list_head(struct akl_list *);
void *akl_list_shift(struct akl_list *);
struct akl_list_entry *
    akl_list_shift_entry(struct akl_list *);
unsigned int akl_list_count(struct akl_list *);

struct akl_value *akl_list_index_value(struct akl_list *, int);
void  *akl_list_last(struct akl_list *);
bool_t akl_list_is_empty(struct akl_list *);
struct akl_list_entry *akl_list_index_entry(struct akl_list *, int);
void  *akl_list_index(struct akl_list *, int);
struct akl_list_entry *akl_list_find(struct akl_list *, akl_cmp_fn_t, void *, unsigned int *);
struct akl_list_entry *
       akl_list_find_value(struct akl_list *, struct akl_value *, unsigned int *);
struct akl_list_entry *
       akl_list_remove_entry(struct akl_list *, struct akl_list_entry *);
struct akl_list_entry *
       akl_list_remove(struct akl_list *, akl_cmp_fn_t, void *);
struct akl_value *akl_duplicate_value(struct akl_state *, struct akl_value *);
struct akl_list *akl_list_duplicate(struct akl_state *, struct akl_list *);
struct akl_list_entry *
       akl_list_pop_entry(struct akl_list *);
void  *akl_list_pop(struct akl_list *);

int    akl_io_getc(struct akl_io_device *);
int    akl_io_ungetc(int, struct akl_io_device *);
bool_t akl_io_eof(struct akl_io_device *dev);

struct akl_context *akl_compile(struct akl_state *s, struct akl_io_device *dev);
struct akl_value   *akl_exec_eval(struct akl_state *s);
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
    enum AKL_ALERT_TYPE  err_type;
    const char          *err_msg;
};

void akl_raise_error(struct akl_context *, enum AKL_ALERT_TYPE, const char *fmt, ...);
void akl_clear_errors(struct akl_state *);
void akl_print_errors(struct akl_state *);

/* Create a new user type and register it for the interpreter. The returned
  integer will identify this new type. */
unsigned int
akl_register_type(struct akl_state *, const char *, akl_gc_destructor_t);
void
akl_deregister_type(struct akl_state *, unsigned int);

enum AKL_INIT_FLAGS {
    AKL_LIB_BASIC       = 0x001,
    AKL_LIB_NUMBERIC    = 0x002,
    AKL_LIB_CONDITIONAL = 0x004,
    AKL_LIB_PREDICATE   = 0x008,
    AKL_LIB_DATA        = 0x010,
    AKL_LIB_SYSTEM      = 0x020,
    AKL_LIB_TIME        = 0x040,
    AKL_LIB_OS          = 0x080,
    AKL_LIB_LOGICAL     = 0x100,
    AKL_LIB_FILE        = 0x200,
    AKL_LIB_ALL = AKL_LIB_BASIC|AKL_LIB_NUMBERIC
        |AKL_LIB_CONDITIONAL|AKL_LIB_PREDICATE|AKL_LIB_DATA
        |AKL_LIB_SYSTEM|AKL_LIB_TIME|AKL_LIB_OS|AKL_LIB_LOGICAL|AKL_LIB_FILE
};

struct akl_module *akl_load_module(struct akl_state *, const char *);
struct akl_module *akl_find_module(struct akl_state *, const char *);
bool_t akl_unload_module(struct akl_state *, const char *, bool_t);

/* These function must be implemented in an os_*.c file */
void akl_init_library(struct akl_state *, enum AKL_INIT_FLAGS);
void akl_init_spec_library(struct akl_state *, enum AKL_INIT_FLAGS);
void akl_declare_functions(struct akl_state *, const struct akl_fun_decl *);
void akl_init_os(struct akl_state *);
void akl_init_file(struct akl_state *);
char *akl_get_module_path(struct akl_state *, const char *);
void akl_module_free(struct akl_state *, struct akl_module *);
struct akl_module *akl_load_module_desc(struct akl_state *, char *);
void akl_free_module(struct akl_state *, struct akl_module *);


#ifndef AKL_CFUN_PREFIX
#define AKL_CFUN_PREFIX akl_fn_
#endif // AKL_CFUN_PREFIX

#ifndef AKL_SFUN_PREFIX
#define AKL_SFUN_PREFIX akl_sfun_
#endif // AKL_SFUN_PREFIX

#define AKL_CAT_EXP(a, b) a ## b
#define AKL_CAT(a, b) AKL_CAT_EXP(a, b)

#define AKL_DEFINE_FUN(fname, ctx, argc) \
    struct akl_value * AKL_CAT(AKL_CFUN_PREFIX, fname)(struct akl_context * ctx, int argc)

#define AKL_DEFINE_NE_FUN(fname, ctx, argc) \
    struct akl_value * AKL_CAT(AKL_CFUN_PREFIX, fname)(struct akl_context * ctx, int argc)

#define AKL_DEFINE_SFUN(fname, ctx) \
    struct akl_value * AKL_CAT(AKL_SFUN_PREFIX, fname)(struct akl_context * ctx)

#define AKL_DECLARE_FUNS(vname) \
    static const struct akl_fun_decl vname[] =

#define AKL_FUN(cfun, name, desc) { AKL_FUNC_CFUN, { AKL_CAT(AKL_CFUN_PREFIX, cfun) }, name, desc }
#define AKL_NE_FUN(cfun, name, desc) { AKL_FUNC_NOEVAL, { AKL_CAT(AKL_CFUN_PREFIX, cfun) }, name, desc }
#define AKL_SFUN(scfun, name, desc) { AKL_FUNC_SPECIAL, { (akl_cfun_t)AKL_CAT(AKL_SFUN_PREFIX, scfun) }, name, desc }
#define AKL_END_FUNS() { 0, { NULL }, NULL, NULL }

#if USE_COLORS
#define AKL_GREEN  "\x1b[32m"
#define AKL_YELLOW "\x1b[33m"
#define AKL_GRAY   "\x1b[30m"
#define AKL_BLUE   "\x1b[34m"
#define AKL_RED    "\x1b[31m"
#define AKL_PURPLE "\x1b[35m"
#define AKL_BRIGHT_GREEN "\x1b[1;32m"
#define AKL_BRIGHT_YELLOW "\x1b[1;33m"
#define AKL_BRIGHT_GRAY   "\x1b[1;30m"
#define AKL_START_COLOR(s,c) if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) printf("%s", (c))
#define AKL_COLORFUL(s,c) (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS) ? c : "")
#define AKL_END_COLORFUL(s) (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS) ? AKL_END_COLOR_MARK : "")
#define AKL_END_COLOR_MARK "\x1b[0m"
#define AKL_END_COLOR(s) if(AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) printf(AKL_END_COLOR_MARK)
#else
#define AKL_GREEN  ""
#define AKL_YELLOW ""
#define AKL_GRAY   ""
#define AKL_BLUE   ""
#define AKL_RED    ""
#define AKL_PURPLE ""
#define AKL_BRIGHT_GREEN ""
#define AKL_BRIGHT_YELLOW ""
#define AKL_BRIGHT_GRAY   ""
#define AKL_START_COLOR(s,c)
#define AKL_COLORFUL(s,c) ""
#define AKL_END_COLORFUL(s) ""
#define AKL_END_COLOR_MARK ""
#define AKL_END_COLOR
#endif

#endif // AKLISP_H
