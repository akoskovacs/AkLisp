#include "aklisp.h"

struct akl_list *akl_parse_list(struct akl_instance *in, struct akl_io_device *dev, bool_t is_q)
{
    struct akl_value *val;
    struct akl_list *list = akl_new_list(in);
    list->is_quoted = is_q;
    int is_quoted = 0;
    token_t tok;
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
            val = akl_new_list_value(in
                , akl_parse_list(in, dev, is_quoted));
            break;

            case tQUOTE:
            is_quoted = 1;
            break;

            case tNIL:
            val = &NIL_VALUE;
            break;

            case tTRUE:
            val = &TRUE_VALUE;
            break;

            default:
            break;
        }
        val->is_quoted = is_quoted;
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
