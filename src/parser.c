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

struct akl_value *akl_parse_value(struct akl_state *in, struct akl_io_device *dev)
{
    bool_t is_quoted = FALSE;
    struct akl_list *l;
    struct akl_value *value;
    token_t tok;
    int quote_count = 0;
    while ((tok = akl_lex(dev))) {
        switch (tok) { 
            case tEOF:
            akl_lex_free();
            case tRBRACE:
            return NULL;

            case tATOM:
            value = akl_new_atom_value(in, akl_lex_get_atom());
            value->is_quoted = is_quoted;
            value->va_lex_info = akl_new_lex_info(in, dev);
            is_quoted = FALSE;
            return value;

            case tNUMBER:
            value = akl_new_number_value(in, akl_lex_get_number());
            value->va_lex_info = akl_new_lex_info(in, dev);
            return value;

            case tSTRING:
            value = akl_new_string_value(in, akl_lex_get_string());
            value->va_lex_info = akl_new_lex_info(in, dev);
            return value;

            /* Whooa new list */
            case tLBRACE:
            l = akl_parse_list(in, dev);
            l->is_quoted = is_quoted;
            is_quoted = FALSE;
            value = akl_new_list_value(in, l);
            value->va_lex_info = akl_new_lex_info(in, dev);
            return value;

            case tQUOTE:
            is_quoted = TRUE;
            continue;

            case tNIL:
            NIL_VALUE.va_lex_info = akl_new_lex_info(in, dev);
            return &NIL_VALUE;

            case tTRUE:
            TRUE_VALUE.va_lex_info = akl_new_lex_info(in, dev);
            return &TRUE_VALUE;

            default:
            break;
            /* TODO: Set the 'is_quote' to false
              when is "not used" */
        }
    }
    return NULL;
}

struct akl_list *akl_parse_list(struct akl_state *in, struct akl_io_device *dev)
{
    struct akl_value *value = NULL;
    struct akl_list *list, *lval, *last_list = NULL;
    list = akl_new_list(in);
    while ((value = akl_parse_value(in, dev)) != NULL) {
        /* If the next value is a list, reparent it... */
        if (AKL_CHECK_TYPE(value, TYPE_LIST)) {
            lval = AKL_GET_LIST_VALUE(value);
            lval->li_parent = list;
        }
        akl_list_append_value(in, list, value);
    }
    return list;
}

struct akl_list *akl_parse_io(struct akl_state *in)
{
    struct akl_io_device *dev = in->ai_device;
    struct akl_value *value = NULL;
    assert(dev);
    in->ai_program = akl_new_list(in);
    while ((value = akl_parse_value(in, dev)))
        akl_list_append_value(in, in->ai_program, value);

    return in->ai_program;
}
