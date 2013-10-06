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

RB_GENERATE(ATOM_TREE, akl_atom, at_entry, cmp_atom);

struct akl_atom *akl_add_global_atom(struct akl_state *in, struct akl_atom *atom)
{
    AKL_GC_SET_STATIC(atom);
    return ATOM_TREE_RB_INSERT(&in->ai_atom_head, atom);
}

void akl_remove_global_atom(struct akl_state *in, struct akl_atom *atom)
{
    ATOM_TREE_RB_REMOVE(&in->ai_atom_head, atom);
}

void akl_remove_function(struct akl_state *in, akl_cfun_t fn)
{
    struct akl_atom *atom;
    RB_FOREACH(atom, ATOM_TREE, &in->ai_atom_head) {
        if (atom->at_value && atom->at_value->va_value.func->fn_body.cfun == fn)
            akl_remove_global_atom(in, atom);
    }
}

static struct akl_function *
akl_add_global_function(struct akl_state *s, const char *name
    , const char *desc)
{
    struct akl_atom *atom = akl_new_atom(s, (char *)name);
    struct akl_function *f = akl_new_function(s);
    if (desc != NULL)
        atom->at_desc = (char *)desc;
    atom->at_value = akl_new_function_value(s, f);

    akl_add_global_atom(s, atom);
    return f;
}


void akl_add_global_cfun(struct akl_state *s, akl_cfun_t fn
    , const char *name, const char *desc)
{
    struct akl_function *fun = akl_add_global_function(s, name, desc);
    fun->fn_type = AKL_FUNC_CFUN;
    fun->fn_body.cfun = fn;
}

void akl_add_global_sfun(struct akl_state *s, akl_sfun_t fn
    , const char *name, const char *desc)
{
    struct akl_function *fun = akl_add_global_function(s, name, desc);
    fun->fn_type = AKL_FUNC_SPECIAL;
    fun->fn_body.scfun = fn;
}

struct akl_atom *
akl_get_global_atom(struct akl_state *in, const char *name)
{
    struct akl_atom *atm, *res;
    if (name == NULL)
        return NULL;

    atm = akl_new_atom(in, strdup(name));
    res = ATOM_TREE_RB_FIND(&in->ai_atom_head, atm);
    return (res == NULL) ? atm : res;
}

void akl_do_on_all_atoms(struct akl_state *in, void (*fn) (struct akl_atom *))
{
    struct akl_atom *atm;
    RB_FOREACH(atm, ATOM_TREE, &in->ai_atom_head) {
       fn(atm);
    }
}

void akl_declare_functions(struct akl_state *s, const struct akl_fun_decl *funs)
{
    assert(s && funs);
    while (funs->fd_fun.cfun && funs->fd_name) {
        switch (funs->fd_type) {
            case AKL_FUNC_CFUN:
            akl_add_global_cfun(s, funs->fd_fun.cfun, funs->fd_name, funs->fd_desc);
            break;

            case AKL_FUNC_SPECIAL:
            akl_add_global_sfun(s, funs->fd_fun.sfun, funs->fd_name, funs->fd_desc);
            break;

            default:
            assert(funs->fd_type);
            break;
        }
        funs++;
    }
}

akl_cfun_t akl_get_global_cfun(struct akl_state *in, const char *name)
{
#if 0
    struct akl_atom *atm = akl_get_global_atom(in, name);
    if (akl_f) {
        if (atm->at_value->va_type == TYPE_CFUN)
            return atm->at_value->va_value.cfunc;
    }
#endif
    return NULL;
}
