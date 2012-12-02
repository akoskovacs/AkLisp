#include <stdio.h>
#include <aklisp.h>

/* Our current type id */
static akl_utype_t file_utype;
/* Standard streams */
static struct akl_atom *akl_stdin, *akl_stdout, *akl_stderr;

static void file_utype_desctruct(struct akl_state *in, void *obj)
{
    if (obj != NULL && obj != stdin 
            && obj != stdout && obj != stderr)
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
        if (fp && fprintf(fp, "%s", AKL_GET_STRING_VALUE(msg)) != -1)
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

static void create_std_desc(struct akl_state *s, akl_utype_t tid)
{
    akl_stdin = akl_new_atom(s, strdup("STDIN"));
    akl_stdout = akl_new_atom(s, strdup("STDOUT"));
    akl_stderr = akl_new_atom(s, strdup("STDERR"));

    akl_stdin->at_value = akl_new_user_value(s, tid, stdin);
    akl_stdin->at_desc = "The descriptor of the standard input";

    akl_stdout->at_value = akl_new_user_value(s, tid, stdout);
    akl_stdout->at_desc = "The descriptor of the standard output";

    akl_stderr->at_value = akl_new_user_value(s, tid, stderr);
    akl_stderr->at_desc = "The descriptor of the standard error stream";

    akl_stdin->at_is_const = akl_stdout->at_is_const
        = akl_stderr->at_is_const = TRUE;

    akl_add_global_atom(s, akl_stdin);
    akl_add_global_atom(s, akl_stdout);
    akl_add_global_atom(s, akl_stderr);
}

static void destroy_std_desc(struct akl_state *s)
{
    akl_remove_global_atom(s, akl_stdin);
    akl_remove_global_atom(s, akl_stdout);
    akl_remove_global_atom(s, akl_stderr);
}

static int file_load(struct akl_state *s)
{
    file_utype = akl_register_type(s, "FILE", file_utype_desctruct);
    create_std_desc(s, file_utype);
    AKL_ADD_CFUN(s, file_open, "OPEN", "Open a file for a given operation (read, write)");
    AKL_ADD_CFUN(s, file_close, "CLOSE", "Close a file");
    AKL_ADD_CFUN(s, file_print, "FPRINT", "Write a string to a stream");
    AKL_ADD_CFUN(s, file_getline, "GETLINE", "Get a line from a stream");
    return AKL_LOAD_OK;
}

static int file_unload(struct akl_state *s)
{
    akl_deregister_type(s, file_utype);
    destroy_std_desc(s);
    AKL_REMOVE_CFUN(s, file_open);
    AKL_REMOVE_CFUN(s, file_close);
    AKL_REMOVE_CFUN(s, file_print);
    AKL_REMOVE_CFUN(s, file_getline);
    return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(file_load, file_unload, "file"
    , "File-handling functions", "Kovacs Akos");
