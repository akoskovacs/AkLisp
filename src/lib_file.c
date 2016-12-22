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
#include <stdio.h>

/* Our current type id */
static akl_utype_t akl_file_utype;
/* Standard streams */
static struct akl_atom *akl_stdin, *akl_stdout, *akl_stderr;

static void file_utype_desctruct(struct akl_state *in, void *obj)
{
    if (obj != NULL && obj != stdin && obj != stdout && obj != stderr) {
        fclose((FILE *)obj);
    }
}

static FILE *
unpack_file_userdata(struct akl_userdata *ud)
{
    if (ud != NULL) {
        return (FILE *)ud->ud_private;
    }
    return NULL;
}


AKL_DEFINE_FUN(file_open, ctx, argc)
{
    FILE *fp = NULL;
    const char *fname = NULL;
    struct akl_symbol *mode;
    if (akl_get_args_strict(ctx, 2, AKL_VT_STRING, &fname, AKL_VT_SYMBOL, &mode) == -1) {
        return AKL_NIL;
    }
    fp = fopen(fname, mode->sb_name);
    if (fp) {
        return akl_new_user_value(in, akl_file_utype, (void *)fp);
    }
    return AKL_NIL;
}

AKL_CFUN_DEFINE(file_close, in, args)
{
    struct akl_userdata *udata;
    FILE *fp;
    if (akl_get_args_strict(ctx, 1, AKL_VT_USERDATA, &udata) == -1) {
        return AKL_NIL;
    }
    if ((fp = unpack_file_userdata(udata)) != NULL) {
        fclose(fp);
        return AKL_TRUE;
    }
    return AKL_NIL;
}

/* A stupid (f)printf() implementation. */
AKL_CFUN_DEFINE(file_printf, s, args)
{
    struct akl_value *val, *a1, *a2;
    struct akl_list_entry *ent;
    FILE *fp = stdout;
    const char *fmt = NULL;
    const char *nf = NULL;
    const char *ferror = "PRINT: Missing format argument for \'%%%c\'.\n";
    ent = args->li_head;
    a1 = AKL_ENTRY_VALUE(ent);
    if (akl_check_user_type(a1, akl_file_utype)) {
        fp = file_get_fp(s, args, akl_stdout, "PRINTF", FALSE);
        ent = ent->le_next;
        a2 = AKL_ENTRY_VALUE(ent);
        if (AKL_CHECK_TYPE(a2, TYPE_STRING)) {
            fmt = AKL_GET_STRING_VALUE(a2);
        }
    } else if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        fmt = AKL_GET_STRING_VALUE(a1);
        fp = stdout;
    } else {
        AKL_LIST_FOREACH(ent, args) {
            akl_print_value(s, AKL_ENTRY_VALUE(ent));
        }
        return &TRUE_VALUE;
    }
    if (!fmt) {
        akl_add_error(s, AKL_ERROR
          , a1->va_lex_info, "ERROR: PRINTF: You must provide a format string.\n");
        return &NIL_VALUE;
    }
    ent = ent->le_next; // Argument after the format string

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': case 'o': case 'x': case 'X':
                if (ent) {
                    val = AKL_ENTRY_VALUE(ent);
                    val = akl_to_number(s, val);
                    if (*fmt == 'd')
                        nf = "%d";
                    else if (*fmt == 'o')
                        nf = "%o";
                    else if (*fmt == 'x')
                        nf = "%x";
                    else
                        nf = "%#x"; // WARNING: The %X has different meaning here!!!
                        fprintf(fp, nf, (int)AKL_GET_NUMBER_VALUE(val));
                } else {
                    akl_add_error(s, AKL_WARNING, a1->va_lex_info, ferror, *fmt);
                }
                break;

                case 'g': case 'f': case 'e':
                if (ent) {
                    val = AKL_ENTRY_VALUE(ent);
                    val = akl_to_number(s, val);
                    fprintf(fp, "%g", AKL_GET_NUMBER_VALUE(val));
                } else {
                    akl_add_error(s, AKL_WARNING, a1->va_lex_info, ferror, *fmt);
                }
                break;

                case 's': case 'c':
                if (ent) {
                    val = AKL_ENTRY_VALUE(ent);
                    val = akl_to_string(s, val);
                    fprintf(fp, "%s", AKL_GET_STRING_VALUE(val));
                } else {
                    akl_add_error(s, AKL_WARNING, a1->va_lex_info, ferror, *fmt);
                }
                break;

                case '%':
                fprintf(fp, "%%");
                break;

                default:
                akl_add_error(s, AKL_WARNING, a1->va_lex_info
                              , "PRINT: Unknown format character '%%%c'.", *fmt);
                break;
            }
            if (ent)
                ent = ent->le_next;
            // TODO: Substitute with the right hexcode and move it to the lexer
        } else if (*fmt == '\\') {
            switch (*++fmt) {
                case 'n': nf = "\n"; break;
                case 'r': nf = "\r"; break;
                case 't': nf = "\t"; break;
                case 'v': nf = "\v"; break;
                case 'a': nf = "\a"; break;
                case 'f': nf = "\f"; break;
                case 'b': nf = "\b"; break;
            }
            fprintf(fp, nf);
        } else {
            fputc(*fmt, fp);
        }
        fmt++;
    }
    return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(file_getline, in, args)
{
    size_t line_s = 0;
    ssize_t n;
    char *line = NULL;
    FILE *fp = file_get_fp(in, args, akl_stdin, "GETLINE", FALSE);
    if (fp && (n = getline(&line, &line_s, fp)) != -1) {
        line[n-1] = '\0';
        return akl_new_string_value(in, line);
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_read_number, in, args)
{
    double num = 0;
    FILE *fp = file_get_fp(in, args, akl_stdin, "READ-NUMBER", FALSE);
    if (fp) {
        fscanf(fp, "%lf", &num);
        return akl_new_number_value(in, num);
    }
    return &NIL_VALUE;
}

#ifdef _GNUC_
AKL_CFUN_DEFINE(file_read_string, in, args)
{
    char *str = NULL;
    FILE *fp = file_get_fp(in, args, akl_stdin, "READ-STRING", FALSE);
    if (fp) {
        fscanf(fp, "%a%s", &str);
        if (fp)
            return akl_new_number_value(in, str);
    }
    return &NIL_VALUE;
}
#else // _GNUC_
AKL_CFUN_DEFINE(file_read_string, in, args)
{
    char *str = (char *)akl_malloc(in, 256);
    FILE *fp = file_get_fp(in, args, akl_stdin, "READ-STRING", FALSE);
    fscanf(fp, "%s", str);
    if (str)
        return akl_new_string_value(in, str);
    return &NIL_VALUE;
}
#endif // _GNUC_

AKL_CFUN_DEFINE(file_newline, in, args)
{
    FILE *fp = file_get_fp(in, args, akl_stdout, "NEWLINE", FALSE);
    if (fp) {
        fprintf(fp, "\n");
        return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_seek, in, args)
{
    struct akl_value *a2, *a3;
    struct akl_atom *sym;
    int pos;
    int whence;
    const char *set[] = { "SEEK_SET", "SEEK-SET", "SET", "BEGIN", NULL };
    const char *cur[] = { "SEEK_CUR", "SEEK-CUR", "CUR", "CURR", "CURRENT", NULL };
    const char *end[] = { "SEEK_END", "SEEK-END", "END",  NULL };

    a2 = AKL_SECOND_VALUE(args);
    a3 = AKL_THIRD_VALUE(args);
    FILE *fp = file_get_fp(in, args, NULL, "FSEEK", TRUE);
    if (fp) {
       if (AKL_CHECK_TYPE(a2, TYPE_NUMBER)
               && AKL_CHECK_TYPE(a3, TYPE_ATOM)) {
           sym = AKL_GET_ATOM_VALUE(a3);
           if (akl_is_equal_with(sym, set))
               whence  = SEEK_SET;
           else if (akl_is_equal_with(sym, cur))
               whence  = SEEK_CUR;
           else if (akl_is_equal_with(sym, end))
               whence  = SEEK_END;
           else {
               akl_add_error(in, AKL_ERROR, a3->va_lex_info
                             , "FSEEK: Whence parameter is not valid.\n");
               return &NIL_VALUE;
           }
           pos = fseek(fp, (long)AKL_GET_NUMBER_VALUE(a2), whence);
           return (pos == 0) ? &TRUE_VALUE : &NIL_VALUE;
       }
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_rewind, in, args)
{
    FILE *fp = file_get_fp(in, args, NULL, "FREWIND", TRUE);
    if (fp) {
        rewind(fp);
        return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(file_tell, in, args)
{
    FILE *fp = file_get_fp(in, args, NULL, "FTELL", TRUE);
    if (fp) {
        return akl_new_number_value(in, (double)ftell(fp));
    }
    return &TRUE_VALUE;
}

AKL_DECLARE_FUNS(file_io_funs) {
    AKL_FUN(file_open, "file-open", "Open a file for a given operation (read, write)"),
    AKL_FUN(file_close, "file-close", "Close a file"),
    AKL_FUN(file_printf, "print", "Write a string to a stream (with C-style formatting)"),
    AKL_FUN(file_getline, "getline", "Get a line from a stream"),
    AKL_FUN(file_read_number, "read-number", "Read a number from the standard input"),
    AKL_FUN(file_read_string, "read-word", "Read a word from the standard input"),
    AKL_FUN(file_read_string, "read-word", "Read a word from the standard input"),
    AKL_FUN(file_newline, "newline", "Just put out a newline character to the standard output"),
    AKL_FUN(file_seek, "file-seek", "Sets the file position"),
    AKL_FUN(file_tell, "file-tell", "Tells the current file position"),
    AKL_FUN(file_rewind, "file-rewind", "Sets the current file position to the beginning of the file"),
    AKL_END_FUNS()
}

void akl_init_file(struct akl_state *s)
{
    akl_file_utype = akl_register_type(s, "FILE", file_utype_desctruct);
    akl_stdin = akl_new_symbol(s, AKL_CSTR("stdin"));
    akl_stout = akl_new_symbol(s, AKL_CSTR("stdout"));
    akl_sterr = akl_new_symbol(s, AKL_CSTR("stderr"));
    akl_declare_functions(s, file_io_funs);
}

#if 0
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
#endif
