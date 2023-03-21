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

const char *akl_type_name[10] = {
    "nil", "true", "symbol", "variable", "number"
  , "string", "list", "function", "userdata", NULL
};

struct akl_value TRUE_VALUE = {
    { AKL_GC_VALUE , FALSE, FALSE , TRUE }
  , NULL, AKL_VT_TRUE, { (double)1 }
  , FALSE, FALSE
};

struct akl_value NIL_VALUE = {
    { AKL_GC_VALUE , FALSE, FALSE , TRUE }
  , NULL, AKL_VT_NIL, { (double)0 }
  , FALSE, TRUE
};

akl_nomem_action_t
akl_def_nomem_handler(struct akl_state *s)
{
    if (akl_gc_tryfree(s))
        return AKL_NM_TRYAGAIN;
    else
        fprintf(stderr, "ERROR! No memory left!\n");

    return AKL_NM_TERMINATE;
}

void akl_init_state(struct akl_state *s, const struct akl_mem_callbacks *cbs)
{
    if (cbs == NULL) {
        s->ai_mem_fn = &akl_mem_std_callbacks;
    } else {
        akl_set_mem_callbacks(s, cbs);
    }
    s->ai_interrupted = FALSE;
    AKL_SET_FEATURE(s, AKL_CFG_USE_COLORS);
    AKL_SET_FEATURE(s, AKL_CFG_USE_GC);
    akl_gc_init(s);

    RB_INIT(&s->ai_symbols);
    RB_INIT(&s->ai_global_vars);
    s->ai_device = NULL;
    akl_init_list(&s->ai_modules);
    akl_init_vector(s, &s->ai_utypes, 5, sizeof(struct akl_module *));
    akl_init_list(&s->ai_stack);
    s->ai_errors   = NULL;
    akl_init_context(&s->ai_context);
    akl_init_os(s);
}

struct akl_state *akl_new_state(const struct akl_mem_callbacks *cbs)
{
    struct akl_state *s;
    void *(*alloc)(size_t);
    if (cbs == NULL || cbs->mc_malloc_fn == NULL) {
         alloc = malloc;
    }
    s = (struct akl_state *)alloc(sizeof(struct akl_state));
    if (s == NULL)
        return NULL;

    akl_init_state(s, cbs);
    return s;
}

void akl_init_list(struct akl_list *list)
{
    if (list == NULL) {
        return;
    }

    AKL_GC_INIT_OBJ(list, AKL_GC_LIST);
    list->li_head   = NULL;
    list->li_last   = NULL;
    list->is_quoted = FALSE;
    list->is_nil    = FALSE;
    list->li_parent = NULL;
    list->li_count  = 0;
}

struct akl_list *akl_new_list(struct akl_state *s)
{
    struct akl_list *list = (struct akl_list *)akl_gc_malloc(s, AKL_GC_LIST);
    akl_init_list(list);
    return list;
}

void akl_init_context(struct akl_context *ctx)
{
    ctx->cx_state     = NULL;
    ctx->cx_dev       = NULL;
    ctx->cx_func      = NULL;
    ctx->cx_func_name = NULL;
    ctx->cx_ir        = NULL;
    ctx->cx_lex_info  = NULL;
    ctx->cx_parent    = NULL;
    ctx->cx_frame     = NULL;
    ctx->cx_stack     = NULL;
    ctx->cx_fn_main   = NULL;
    ctx->cx_frame_len = 0;
}

void
akl_init_frame(struct akl_context *ctx, int len)
{
    int fs = len;
    struct akl_list_entry *it = akl_list_it_end(ctx->cx_stack);

    ctx->cx_frame = akl_new_list(ctx->cx_state);
    if (fs == -1) {
        return;
    }

    ctx->cx_frame->li_count = len;
    ctx->cx_frame_len = len;
    ctx->cx_frame->li_last = it;
    while (--fs && akl_list_it_has_prev(it)) {
        akl_list_it_prev(&it);
    }
    ctx->cx_frame->li_head = it;
}

struct akl_context *
akl_new_context(struct akl_state *s)
{
    struct akl_context *ctx = AKL_MALLOC(s, struct akl_context);
    akl_init_context(ctx);
    ctx->cx_state = s;
    return ctx;
}

static struct akl_symbol *
get_or_create_symbol(struct akl_state *s, char *name)
{
    struct akl_symbol *sym = akl_get_symbol(s, name);

    if (sym == NULL) {
        sym = AKL_MALLOC(s, struct akl_symbol);
        if (sym) {
            sym->sb_name    = name; /* SYM_TREE_RB_INSERT() needs this */
            sym->sb_is_cdef = TRUE;
            SYM_TREE_RB_INSERT(&s->ai_symbols, sym);
            /* The caller has to know, that this is a new symbol */
            sym->sb_name    = NULL;
        }
    }
    return sym;
}

struct akl_symbol *
akl_new_symbol(struct akl_state *s, char *name, bool_t is_cname)
{
    struct akl_symbol *sym = get_or_create_symbol(s, name);
    /* If the symbol already exsisted, no need to modifiy it's name */
    if (sym != NULL && sym->sb_name == NULL) {
        sym->sb_name    = name;
        /* If the string is a constant char array, no free() called. */
        sym->sb_is_cdef = is_cname;
    }
    return sym;
}

struct akl_symbol *
akl_get_symbol(struct akl_state *s, char *name)
{
    struct akl_symbol sym, *sptr;
    sym.sb_name = name;
    sptr = SYM_TREE_RB_FIND(&s->ai_symbols, &sym);
    return sptr;
}

/* Only copies the string, when a new symbol is created */
struct akl_symbol *
akl_get_or_create_symbol(struct akl_state *s, char *name)
{
    struct akl_symbol *sym = get_or_create_symbol(s, name);
    if (sym != NULL && sym->sb_name == NULL) {
        sym->sb_name    = AKL_STRDUP(name);
        sym->sb_is_cdef = FALSE;
    }
    return sym;
}

struct akl_variable *
akl_new_var(struct akl_state *s, struct akl_symbol *sym)
{
    struct akl_variable *var = (struct akl_variable *)
            akl_gc_malloc(s, AKL_GC_VARIABLE);
    AKL_GC_INIT_OBJ(var, AKL_GC_VARIABLE);
    var->vr_symbol = sym;
    var->vr_desc   = NULL;
    var->vr_value  = NULL;
    return var;
}

struct akl_variable *
akl_new_variable(struct akl_state *s, char *name, bool_t is_cname)
{
    struct akl_symbol *sym = akl_new_symbol(s, name, is_cname);
    struct akl_variable *v = akl_new_var(s, sym);
    return v;
}

struct akl_list_entry *
akl_new_list_entry(struct akl_state *s)
{
    struct akl_list_entry *ent =
            (struct akl_list_entry *)akl_gc_malloc(s, AKL_GC_LIST_ENTRY);
    AKL_GC_INIT_OBJ(ent, AKL_GC_LIST_ENTRY);
    ent->le_data = NULL;
    ent->le_next = NULL;
    ent->le_prev = NULL;
    return ent;
}

struct akl_lex_info *akl_new_lex_info(struct akl_state *in, struct akl_io_device *dev)
{
    struct akl_lex_info *info = AKL_MALLOC(in, struct akl_lex_info);
    if (dev && info) {
        info->li_line = dev->iod_line_count;
        /* The column, where the token start */
        info->li_count = dev->iod_column;
        info->li_name = dev->iod_name;
    }
    return info;
}

struct akl_value *akl_new_value(struct akl_state *s)
{
    struct akl_value *val = (struct akl_value *)akl_gc_malloc(s, AKL_GC_VALUE);
    AKL_GC_INIT_OBJ(val, AKL_GC_VALUE);
    val->is_nil      = FALSE;
    val->is_quoted   = FALSE;
    val->va_lex_info = NULL;
    return val;
}

struct akl_value *akl_new_string_value(struct akl_state *in, char *str)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = AKL_VT_STRING;
    val->va_value.string = str;
    val->is_nil = FALSE;
    return val;
}

struct akl_value *akl_new_number_value(struct akl_state *in, double num)
{
    struct akl_value *val = akl_new_value(in);
    val->va_type = AKL_VT_NUMBER;
    val->va_value.number = num;
    return val;
}

struct akl_value *akl_new_list_value(struct akl_state *s, struct akl_list *lh)
{
    struct akl_value *val = akl_new_value(s);
    assert(lh != NULL);
    val->va_type = AKL_VT_LIST;
    val->va_value.list = lh;
    return val;
}

struct akl_value *akl_new_nil_value(struct akl_state *s)
{
    struct akl_value *nil = akl_new_value(s);
    *nil = NIL_VALUE;
    return nil;
}

struct akl_value *akl_new_true_value(struct akl_state *s)
{
    struct akl_value *t = akl_new_value(s);
    *t = TRUE_VALUE;
    return t;
}

struct akl_value *akl_new_user_value(struct akl_state *s, unsigned int type, void *data)
{
    struct akl_userdata *udata;
    struct akl_value    *value;
    /* We should stop now, since the requested type does not exist */
    assert(akl_vector_at(&s->ai_utypes, type));
    udata = (struct akl_userdata *)akl_gc_malloc(s, AKL_GC_UDATA);
    value = akl_new_value(s);
    udata->ud_id = type;
    udata->ud_private = data;
    value->va_type = AKL_VT_USERDATA;
    value->va_value.udata = udata;
    return value;
}

struct akl_value *
akl_new_sym_value(struct akl_state *s, struct akl_symbol *sym)
{
    struct akl_value *val = akl_new_value(s);
    AKL_ASSERT(sym, NULL);
    AKL_ASSERT(val, NULL);

    val->va_type          = AKL_VT_SYMBOL;
    val->va_value.symbol  = sym;
    return val;
}

struct akl_value *
akl_new_symbol_value(struct akl_state *s, char *name, bool_t is_cname)
{
    struct akl_value *val = akl_new_value(s);
    AKL_ASSERT(val, NULL);
    struct akl_symbol *sym  = akl_new_symbol(s, name, is_cname);
    return akl_new_sym_value(s, sym);
}

/** @brief Create a value for variable
  * @param state Current interpreter state
  * @param name  Name of the new variable
  * @param is_cname True if the name is a constant string (no free())
  * Special akl_new_*_value() function, it does point to struct akl_variable.
  * Instead it will point to a symbol (the name of variable) as AKL_VT_VARIABLE type.
*/
struct akl_value *
akl_new_variable_value(struct akl_state *s, char *name, bool_t is_cname)
{
    struct akl_value *sval;
    sval          = akl_new_symbol_value(s, name, is_cname);
    sval->va_type = AKL_VT_VARIABLE;
    return sval;
}

struct akl_function *
akl_new_function(struct akl_state *s /*, enum AKL_FUNCTION_TYPE ftype*/)
{
    struct akl_function *f;
    f = (struct akl_function *)akl_gc_malloc(s, AKL_GC_FUNCTION);
    AKL_GC_INIT_OBJ(f, AKL_GC_FUNCTION);
//    f->fn_type = ftype;
    f->fn_body.cfun = NULL;
    return f;
}

struct akl_value *
akl_new_function_value(struct akl_state *s, struct akl_function *f)
{
    struct akl_value *v = akl_new_value(s);
    v->va_type = AKL_VT_FUNCTION;
    v->va_value.func = f;
    return v;
}

struct akl_label *
akl_new_label(struct akl_context *ctx)
{
    return AKL_MALLOC(ctx->cx_state, struct akl_label);
}

void akl_init_label(struct akl_label *l, int ind)
{
    l->la_ir     = NULL;
    l->la_branch = NULL;
    l->la_name   = NULL;
    l->la_ind    = ind;
}

struct akl_list *
akl_new_labels(struct akl_context *ctx, int *loff, int n)
{
    struct akl_label *l;
    struct akl_lisp_fun *uf;
    int i, cnt;
    assert(ctx && ctx->cx_state && ctx->cx_comp_func);
    uf = &ctx->cx_comp_func->fn_body.ufun;
    cnt = akl_list_count(&uf->uf_labels);
    if (loff != NULL) {
        *loff = cnt;
    }
    for (i = 0; i < n; i++) {
        l = akl_new_label(ctx);
        if (l == NULL) {
            return NULL;
        }
        akl_init_label(l, i+cnt);
        akl_list_append(ctx->cx_state, &uf->uf_labels, l);
    }
    return &uf->uf_labels;
}

struct akl_io_device *
akl_new_file_device(struct akl_state *s, const char *file_name, FILE *fp)
{
    assert(s);
    struct akl_io_device *dev;

    dev = AKL_MALLOC(s, struct akl_io_device);
    dev->iod_type        = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos         = 0;
    dev->iod_line_count  = 1;
    dev->iod_name        = file_name;
    dev->iod_buffer      = NULL;
    dev->iod_buffer_size = 0;
    dev->iod_state       = s;
    return dev;
}

struct akl_io_device *
akl_new_string_device(struct akl_state *s, const char *name, const char *str)
{
    assert(s);
    struct akl_io_device *dev;

    dev = AKL_MALLOC(s, struct akl_io_device);
    dev->iod_type        = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos         = 0;
    dev->iod_line_count  = 1;
    dev->iod_char_count  = 0;
    dev->iod_name        = name;
    dev->iod_buffer      = NULL;
    dev->iod_buffer_size = 0;
    dev->iod_state       = s;
    return dev;
}

struct akl_state *
akl_new_file_interpreter(const char *file_name, FILE *fp, const struct akl_mem_callbacks *cbs)
{
    struct akl_state *s = akl_new_state(cbs);
    s->ai_device = akl_new_file_device(s, file_name, fp);
    return s;
}

struct akl_state *
akl_new_string_interpreter(const char *name, const char *str, const struct akl_mem_callbacks *cbs)
{
    struct akl_state *s = akl_new_state(cbs);
    s->ai_device = akl_new_string_device(s, name, str);
    return s;
}

struct akl_state *
akl_reset_string_interpreter(struct akl_state *in, const char *name, const char *str
                             , const struct akl_mem_callbacks *cbs)
{
   if (in == NULL) {
       return akl_new_string_interpreter(name, str, cbs);
   } else if (in->ai_device == NULL) {
       in->ai_device = akl_new_string_device(in, name, str);
       return in;
   } else {
       in->ai_device->iod_type = DEVICE_STRING;
       in->ai_device->iod_source.string = str;
       in->ai_device->iod_pos        = 0;
       in->ai_device->iod_char_count = 0;
       in->ai_device->iod_line_count = 0;
//       if (in->ai_program)
//           akl_free_list(in, in->ai_program);
       return in;
   }
}

/* Won't call fclose() */
struct akl_state *
akl_reset_file_interpreter(struct akl_state *in, const char *name, FILE *fp)
{
   if (in && in->ai_device == NULL) {
       in->ai_device = akl_new_file_device(in, name, fp);
       return in;
   } else {
       in->ai_device->iod_type = DEVICE_FILE;
       in->ai_device->iod_source.string = name;
       in->ai_device->iod_pos        = 0;
       in->ai_device->iod_char_count = 0;
       in->ai_device->iod_line_count = 0;
//       if (in->ai_program)
//           akl_free_list(in, in->ai_program);
       return in;
   }
}

/* ~~~===### Some getter and conversion functions ###===~~~ */

bool_t akl_var_is_function(struct akl_variable *var)
{
    struct akl_value *v = (var != NULL) ? var->vr_value: NULL;
    return (v != NULL) && (v->va_type == AKL_VT_FUNCTION);
}

struct akl_function *
akl_var_to_function(struct akl_variable *var)
{
    struct akl_value *v = (var != NULL) ? var->vr_value: NULL;
    if ((v != NULL) && (v->va_type == AKL_VT_FUNCTION))
        return v->va_value.func;
    return NULL;
}

/* NOTE: These functions give back a NULL pointer, if the conversion
  cannot be completed */
struct akl_value *akl_to_number(struct akl_state *in, struct akl_value *v)
{
    char *str = NULL;
    if (v) {
        switch (v->va_type) {
            case AKL_VT_STRING:
            str = AKL_GET_STRING_VALUE(v);
            break;

            case AKL_VT_SYMBOL:
            if (v->va_value.symbol != NULL) {
                str = v->va_value.symbol->sb_name;
            }
            break;

            case AKL_VT_NIL:
            str = "0";
            break;

            case AKL_VT_TRUE:
            str = "1";
            break;

            case AKL_VT_NUMBER:
            return v;

            default:
            break;
        }
        if (str)
            return akl_new_number_value(in, atof(str));
    }
    return NULL;
}

char *akl_num_to_str(struct akl_state *s, double number)
{
    int strsize = 30;
    char *str = (char *)akl_alloc(s, strsize);
    while (snprintf(str, strsize, "%g", number) >= strsize) {
        strsize += strsize/2;
        str = (char *)akl_realloc(s, str, strsize);
    }
    return str;
}

struct akl_value *akl_to_string(struct akl_state *in, struct akl_value *v)
{
    const char *str;
    if (v) {
        switch (v->va_type) {
            case AKL_VT_NUMBER:
            str = akl_num_to_str(in, AKL_GET_NUMBER_VALUE(v));
            break;

            case AKL_VT_SYMBOL:
            if (v->va_value.symbol != NULL) {
                str = v->va_value.symbol->sb_name;
            }
            break;

            case AKL_VT_NIL:
            str = "NIL";
            break;

            case AKL_VT_TRUE:
            str = "T";
            break;

            case AKL_VT_STRING:
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
    struct akl_value *val;
    char *name = NULL;
    if (v) {
        switch (v->va_type) {
            case AKL_VT_NUMBER:
            name = akl_num_to_str(in, AKL_GET_NUMBER_VALUE(v));
            break;

            case AKL_VT_STRING:
            /* TODO: Eliminate the strup()s */
            name = AKL_GET_STRING_VALUE(v);
            if (name == NULL)
                return NULL;
            name = strdup(name);
            break;

            case AKL_VT_NIL:
            name = strdup("NIL");
            break;

            case AKL_VT_TRUE:
            name = strdup("T");
            break;

            case AKL_VT_SYMBOL:
            /* No need to duplicate a symbol */
            return v;

            default:
            break;
        }
        if (name) {
            val = akl_new_symbol_value(in, name, FALSE);
            val->is_quoted = TRUE;
            return val;
        }
    }
    return NULL;
}

/* ~~~===### UTYPE ###===~~~ */

struct akl_userdata *akl_get_userdata_value(struct akl_value *value)
{
    struct akl_userdata *data;
    if (AKL_CHECK_TYPE(value, AKL_VT_USERDATA)) {
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

#if 0
struct akl_module *akl_get_module_descriptor(struct akl_state *in, struct akl_value *v)
{
    struct akl_userdata *data;
    if (in && AKL_CHECK_TYPE(v, TYPE_USERDATA)) {
       data = akl_get_userdata_value(v);
       return akl_vector_at(&in->ai_modules, data->ud_id);
    }
    return NULL;
}
#endif

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
    struct akl_utype *type = (struct akl_utype *)akl_vector_reserve(&s->ai_utypes);
    type->ut_name = name;
    type->ut_de_fun = de_fun;

    return akl_vector_count(&s->ai_utypes);
}

void akl_deregister_type(struct akl_state *s, unsigned int type)
{
    // TODO: Rewrite it as list
    //AKL_FREE(akl_vector_remove(&s->ai_utypes, type));
}


static int utype_finder_name(void *t, void *name)
{
    struct akl_utype *type = (struct akl_utype *)t;
    return strcmp(type->ut_name, (const char *)name);
}

int akl_get_typeid(struct akl_state *in, const char *tname)
{
    struct akl_utype *utype = NULL;
    int ind;
    utype = (struct akl_utype *)
        akl_vector_find(&in->ai_utypes, utype_finder_name, (void *)tname, &ind);

    return ind;
}
