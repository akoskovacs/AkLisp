#include <stdio.h>
#include <aklisp.h>

/* Our current type id */
static unsigned int file_utype;

static void file_utype_desctruct(struct akl_instance *in, void *obj)
{
    if (obj != NULL)
        fclose((FILE *)obj);
}

AKL_CFUN_DEFINE(file_open, in, args)
{
    FILE *fp;
    struct akl_value *fname = AKL_FIRST_VALUE(args);
    struct akl_value *opts = AKL_SECOND_VALUE(args);
    if (AKL_CHECK_TYPE(fname, TYPE_STRING)
            && AKL_CHECK_TYPE(opts, TYPE_STRING)) {
       fp = fopen(AKL_GET_STRING_VALUE(fname), AKL_GET_STRING_VALUE(opts));     
       if (fp)
           return akl_new_user_value(in, file_utype, (void *)fp);
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_close, in, args)
{
    struct akl_value *fval = AKL_FIRST_VALUE(args);
    struct akl_userdata *udata;
    FILE *fp;
    if (akl_check_user_type(fval, file_utype)) {
        udata = akl_get_userdata_value(fval);
        fp = (FILE *)udata->ud_private;
        if (fclose(fp) == 0) {
            udata->ud_private = NULL;
            return &TRUE_VALUE;
        }
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_print, in, args)
{
    struct akl_value *fval = AKL_FIRST_VALUE(args);
    struct akl_value *msg = AKL_SECOND_VALUE(args);
    FILE *fp;
    if (akl_check_user_type(fval, file_utype)) {
        fp = (FILE *)akl_get_udata_value(fval);
        if (fp && fprintf(fp, AKL_GET_STRING_VALUE(msg)) != -1)
            return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_getline, in, args)
{
    struct akl_value *fval = AKL_FIRST_VALUE(args);
    size_t line_s = 0;
    ssize_t n;
    char *line = NULL;
    FILE *fp;
    if (akl_check_user_type(fval, file_utype)) {
        fp = (FILE *)akl_get_udata_value(fval);
        if (fp && (n = getline(&line, &line_s, fp)) != -1) {
            line[n-1] = '\0';
            return akl_new_string_value(in, line);
        }
    }
    return &NIL_VALUE;
}

static int file_load(struct akl_instance *in)
{
    file_utype = akl_register_type(in, "FILE", file_utype_desctruct);
    AKL_ADD_CFUN(in, file_open, "OPEN", "Open a file for a given operation (read, write)");
    AKL_ADD_CFUN(in, file_close, "CLOSE", "Close a file");
    AKL_ADD_CFUN(in, file_print, "FPRINT", "Write a string to a file");
    AKL_ADD_CFUN(in, file_getline, "GETLINE", "Get a line from a file");
    return AKL_LOAD_OK;
}

static int file_unload(struct akl_instance *in)
{
    akl_deregister_type(in, file_utype);
    AKL_REMOVE_CFUN(in, file_open);
    AKL_REMOVE_CFUN(in, file_close);
    AKL_REMOVE_CFUN(in, file_print);
    AKL_REMOVE_CFUN(in, file_print);
    return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(file_load, file_unload, "file"
    , "File-handling functions", "Kovacs Akos");
