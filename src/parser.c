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

struct akl_list *akl_parse_list(struct akl_instance *in, struct akl_io_device *dev, bool_t is_q)
{
    struct akl_value *val;
    struct akl_list *list, *l;
    int is_quoted = 0;
    token_t tok;
    list = akl_new_list(in);
    list->is_quoted = is_q;
    while ((tok = akl_lex(dev))) {
        switch (tok) {
            case tEOF: case tRBRACE:
            goto list_end;
            break;

            case tATOM:
            val = akl_new_atom_value(in, akl_lex_get_atom());
            break;
            
            case tNUMBER:
            val = akl_new_number_value(in, akl_lex_get_number());
            break;

            case tSTRING:
            val = akl_new_string_value(in, akl_lex_get_string());
            break;

            /* Whooa new list */
            case tLBRACE:
            l = akl_parse_list(in, dev, is_quoted);
            l->li_parent = list;
            val = akl_new_list_value(in, l);
            break;

            case tQUOTE:
            is_quoted = 1;
            continue;

            case tNIL:
            val = &NIL_VALUE;
            break;

            case tTRUE:
            val = &TRUE_VALUE;
            break;

            default:
            break;
        }
        if (is_quoted) {
            val->is_quoted = TRUE;
            is_quoted = FALSE;
        }
        akl_list_append(in, list, val);
    }

list_end:
    if (list->li_elem_count == 0) {
        akl_free_list(in, list);
        return &NIL_LIST;
    } 
    return list;
}

struct akl_list *akl_parse_io(struct akl_instance *in)
{
    struct akl_io_device *dev = in->ai_device;
    assert(dev);
    in->ai_program = akl_parse_list(in, dev, FALSE);
    return in->ai_program;
}
