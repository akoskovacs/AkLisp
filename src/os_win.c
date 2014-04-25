#include <windows.h>
#include "aklisp.h"

int init_os(struct akl_state *s)
{
}

struct akl_module *akl_load_module_desc(struct akl_state *s, const char *libname)
{
    struct akl_module *mod = NULL;
    HINSTANCE dll;
    dll = LoadLibrary(libname);
    if (dll != NULL)
        mod = (struct akl_module *)GetProcAddress(dll, "__module_desc");
    else 
        FreeLibrary(dll);

    return mdesc;
}

void akl_free_module(struct akl_state *s, struct akl_module *mod)
{
    if (mod->am_handle)
        FreeLibrary(mod->am_handle);
    
    akl_free(s, mod->am_path);
}
