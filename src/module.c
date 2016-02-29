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

int module_finder_name(void *m, void *name)
{
    struct akl_module *mod = (struct akl_module *)m;
    if (mod && mod->am_name)
        return strcmp(mod->am_name, (const char *)name);
    return 1;
}

struct akl_module *
akl_find_module(struct akl_state *s, const char *modname)
{
   return (struct akl_module *)akl_list_find(&s->ai_modules
                               , module_finder_name, (void *)modname, NULL)->le_data;
}

struct akl_module *
akl_load_module(struct akl_state *s, const char *modname)
{
    char *mod_path;
    struct akl_module *mod;
    int errcode;
    const char **dp;

    if (akl_find_module(s, modname) != NULL) {
       return NULL;
    }

    if ((mod_path = akl_get_module_path(s, modname)) == NULL) {
        return NULL;
    }
     
    mod = akl_load_module_desc(s, mod_path);

    if (mod && mod->am_name) {
        if (mod->am_depends_on) {
            /* Load dependencies */
            for (dp = mod->am_depends_on; dp != NULL; dp++) {
                if (akl_load_module(s, *dp) == NULL)
                   goto unload_mod;
            }
        }
        /* Start the loader... */
        if (mod->am_load) {
            errcode = mod->am_load(s);
            if (errcode != AKL_LOAD_OK) {
               goto unload_mod;
            }
        }
        if (mod->am_funs) {
            akl_declare_functions(s, mod->am_funs);
        }
        assert(mod->am_load || mod->am_funs);
    } else {
       goto unload_mod;
    }

    akl_list_append(s, &s->ai_modules, mod);
    return mod;

unload_mod:
    akl_free_module(s, mod);
    return NULL;
}

bool_t akl_unload_module(struct akl_state *s, const char *modname, bool_t use_force)
{
    struct akl_list_entry *ent;
    struct akl_module *mod;
    int errcode;

    ent = akl_list_find(&s->ai_modules, module_finder_name, (void *)modname, NULL);
    mod = ent ? (struct akl_module *)ent->le_data : NULL;

    if (!mod) {
        return FALSE;
    }

    if (mod->am_unload == NULL) {
        goto delete_mod;
    }

    errcode = mod->am_unload(s);
    if (errcode == AKL_LOAD_FAIL) {
        if (use_force)
            goto delete_mod;
        return FALSE;
    }

delete_mod:
    akl_list_remove_entry(&s->ai_modules, ent);
    akl_free_module(s, mod);
    return TRUE; 
}
