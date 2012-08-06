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
#include <ctype.h>

#include "aklisp.h"

static char buffer[256];

int akl_io_getc(struct akl_io_device *dev)
{
    int ch;
    if (dev == NULL)
        return EOF;

    switch (dev->iod_type) {
        case DEVICE_FILE:
        ch = fgetc(dev->iod_source.file);
        break;

        case DEVICE_STRING:
        ch = dev->iod_source.string[dev->iod_pos];
        dev->iod_pos++;
        break;
    }
    return ch;
}

int akl_io_ungetc(int ch, struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;
    switch (dev->iod_type) {
        case DEVICE_FILE:
        return ungetc(ch, dev->iod_source.file);

        case DEVICE_STRING:
        dev->iod_pos--;
        return dev->iod_source.string[dev->iod_pos];
    }
    return 0;
}

bool_t akl_io_eof(struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;

    switch (dev->iod_type) {
        case DEVICE_FILE:
        return feof(dev->iod_source.file);

        case DEVICE_STRING:
        return dev->iod_source.string[dev->iod_pos] == 0 ? 1 : 0;
    }
}

size_t copy_number(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);
    if (buffer[0] == '+' || buffer[0] == '-')
        i++;

    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(dev))
            break;
        if (isdigit(ch)) {
            buffer[i] = ch;
            buffer[++i] = '\0';
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

size_t copy_string(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(dev))
            break;
        if (ch != '\"') {
            buffer[i] = ch;
            buffer[++i] = '\0';
        } else {
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

size_t copy_atom(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;

    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(dev))
            break;
        if (ch != ' ' && ch != ')' && ch != '\n') {
            buffer[i] = toupper(ch); /* Good old times... */
            buffer[++i] = '\0';
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

token_t akl_lex(struct akl_io_device *dev)
{
    int ch;
    int i;
    int op = 0;
    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (ch == EOF) {
            return tEOF;
        } else if (ch == '+' || ch == '-') {
            op = ch;
        } else if (isdigit(ch)) {
            akl_io_ungetc(ch, dev);
            if (op != 0) {
                if (op == '+')
                    strcpy(buffer, "+");
                else
                    strcpy(buffer, "-");
                op = 0;
            }
            copy_number(dev);
            return tNUMBER;
        } else if (ch == ' ' || ch == '\n') {
            if (op != 0) {
                if (op == '+')
                    strcpy(buffer, "+");
                else
                    strcpy(buffer, "-");
                op = 0;
                return tATOM;
            } else {
                continue;
            }
        } else if (ch == '"') {
            /* No akl_io_ungetc() */
            copy_string(dev); 
            return tSTRING;
        } else if (ch == '(') {
            return tLBRACE;
        } else if (ch == ')') {
            return tRBRACE;
        } else if (ch == '\'' || ch == ':') {
            return tQUOTE;
        } else if (ch == ';') {
            while ((ch = akl_io_getc(dev)) != '\n') {
                if (akl_io_eof(dev))
                    return tEOF;
            }
        } else if (isalpha(ch) || ispunct(ch)) {
            akl_io_ungetc(ch, dev);
            copy_atom(dev);
            if ((strcasecmp(buffer, "NIL")) == 0)
                return tNIL;
            else if ((strcasecmp(buffer, "T")) == 0)
                return tTRUE;
            else
                return tATOM;
        } else {
            continue;
        }
    }
    return tEOF;
}

char *akl_lex_get_string(void)
{
    return strdup(buffer);   
}

char *akl_lex_get_atom(void)
{
    char *str = strdup(buffer);
    buffer[0] = '\0';
    return str;
}

int akl_lex_get_number(void)
{
    return atoi(buffer);
}
