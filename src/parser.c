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

void
akl_set_lex_info(struct akl_state *s, struct akl_io_device *dev, struct akl_value *value)
{
    assert(value);
    value->va_lex_info = akl_new_lex_info(s, dev);
}

struct akl_value *
akl_parse_token(struct akl_state *s, struct akl_io_device *dev
                , akl_token_t tok, bool_t is_quoted)
{
    struct akl_value *value = NULL;
    struct akl_list *l = NULL;
    switch (tok) {
        case tEOF:
        akl_lex_free(dev);
        case tRBRACE:
        return NULL;

        case tATOM:
        value = akl_new_atom_value(s, akl_lex_get_atom(dev));
        break;

        case tNUMBER:
        value = akl_new_number_value(s, akl_lex_get_number(dev));
        break;

        case tSTRING:
        value = akl_new_string_value(s, akl_lex_get_string(dev));
        break;

        /* We should care only about quoted lists */
        case tLBRACE:
        l = akl_parse_list(s, dev);
        value = akl_new_list_value(s, l);
        break;

        case tNIL:
        value = akl_new_nil_value(s);
        break;

        case tTRUE:
        value = akl_new_true_value(s);
        break;

        case tQUOTE:
        // TODO: Error
        break;
    }

    value->is_quoted = is_quoted;
    akl_set_lex_info(s, dev, value);
    return value;
}

struct akl_value *akl_parse_value(struct akl_state *s, struct akl_io_device *dev)
{
    akl_token_t tok = akl_lex(dev);
    bool_t is_quoted = FALSE;

    if (tok == tQUOTE) {
        is_quoted = TRUE;
        tok = akl_lex(dev);
    }
    return akl_parse_token(s, dev, tok, is_quoted);
}

/* TODO: Fix for NULL */
struct akl_list *akl_parse_list(struct akl_state *in, struct akl_io_device *dev)
{
    struct akl_value *value = NULL, *v = NULL;
    struct akl_list *list, *lval;
    list = akl_new_list(in);
    while ((v = akl_parse_value(in, dev)) != NULL) {

        /* If the next value is a list, reparent it... */
        if (AKL_CHECK_TYPE(value, TYPE_LIST)) {
            lval = AKL_GET_LIST_VALUE(value);
            lval->li_parent = list;
        }
        akl_list_append_value(in, list, value);
    }
    return list;
}

struct akl_list *akl_parse_string(struct akl_state *s, const char *name, const char *str)
{
    struct akl_io_device *dev = akl_new_string_device(name, str, malloc);
    struct akl_list *l = akl_parse_list(s, dev);
    AKL_FREE(s,dev);
    return l;

}

#if 0
struct akl_list *akl_parse_io(struct akl_state *s, struct akl_io_device *dev)
{
    assert(dev);
    akl_token_t tok = akl_lex(s, dev);
    if (tok == tQUOTE)
        return akl_parse_quoted_list(s, dev);

    akl_compile_list(s, dev, &s->ai_ir_code);
    return NULL;
}

struct akl_list *akl_parse(struct akl_state *s)
{
    if (s) {
        return s->ai_program = akl_parse_io(s, s->ai_device);
    }
    return NULL;
}
#endif
