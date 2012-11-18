#include <aklisp.h>
#include <ncurses.h>

AKL_CFUN_DEFINE(curs_getch, in, args)
{
    int ch;
    const char *sname;
    struct akl_value *sym;
    ch = getch();
    switch (ch) {
        case KEY_UP: sname = "KEY_UP"; break;
        case KEY_DOWN: sname = "KEY_DOWN"; break;
        case KEY_LEFT: sname = "KEY_LEFT"; break;
        case KEY_RIGHT: sname = "KEY_RIGHT"; break;
        case KEY_STAB: sname = "KEY_STAB"; break;
        case KEY_HOME: sname = "KEY_HOME"; break;
        case KEY_BACKSPACE: sname = "KEY_BACKSPACE"; break;
        case KEY_EXIT: sname = "KEY_EXIT"; break;
        case KEY_ENTER: sname = "KEY_ENTER"; break;
    }
    sym = akl_new_atom_value(in, strdup(sname));
    sym->is_quoted = TRUE;
    AKL_GC_INC_REF(sym);
    return sym;
}

AKL_CFUN_DEFINE(curs_printw, in, args)
{
    struct akl_value *astr;
    struct akl_value *ay, *ax;
    int x, y, n;
    if (args->li_elem_count > 1) {
        ax = AKL_FIRST_VALUE(args);
        ay = AKL_SECOND_VALUE(args);
        astr = AKL_THIRD_VALUE(args);
        if (AKL_CHECK_TYPE(ax, TYPE_NUMBER) && AKL_CHECK_TYPE(ay, TYPE_NUMBER)
            && AKL_CHECK_TYPE(astr, TYPE_STRING)) {
            x = (int)AKL_GET_NUMBER_VALUE(ax);
            y = (int)AKL_GET_NUMBER_VALUE(ay);
            n = mvprintw(y, x, AKL_GET_STRING_VALUE(astr));
        } else {
            akl_add_error(in, AKL_ERROR, astr->va_lex_info
                , "ERROR: printw: Usage (PRINTW x y message)\n");
            return &NIL_VALUE;
        }
    } else {
        astr = AKL_FIRST_VALUE(args);
        if (AKL_CHECK_TYPE(astr, TYPE_STRING)) {
            n = printw("%s", AKL_GET_STRING_VALUE(astr));
        } else {
            akl_add_error(in, AKL_ERROR, astr->va_lex_info
                , "ERROR: printw: Usage (PRINTW message)\n");
            return &NIL_VALUE;
        }
    }
    return akl_new_number_value(in, n);
}

AKL_CFUN_DEFINE(curs_getmax, in, args)
{
    struct akl_list *list = akl_new_list(in);
    int x, y;
    getmaxyx(stdscr, y, x);
    akl_list_append(in, list, akl_new_number_value(in, x));
    akl_list_append(in, list, akl_new_number_value(in, y));
    list->is_quoted = TRUE;
    return akl_new_list_value(in, list);
}

AKL_CFUN_DEFINE(curs_getxy, in, args)
{
    struct akl_list *list = akl_new_list(in);
    int x, y;
    getyx(stdscr, y, x);
    akl_list_append(in, list, akl_new_number_value(in, x));
    akl_list_append(in, list, akl_new_number_value(in, y));
    list->is_quoted = TRUE;
    return akl_new_list_value(in, list);
}

AKL_CFUN_DEFINE(curs_init_scr, in, args)
{
    initscr();
    return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(curs_endwin, in, args)
{
    endwin();
    return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(curs_refresh, in, args)
{
    refresh();
    return &TRUE_VALUE;
}

AKL_CFUN_DEFINE(curs_noecho, in, args)
{
    noecho();
    return &TRUE_VALUE;
}


static int load_curses_mod(struct akl_state *in)
{
    AKL_ADD_CFUN(in, curs_init_scr, "INITSCR", "Initialize terminal screen");
    AKL_ADD_CFUN(in, curs_endwin, "ENDWIN", "End curses mode");
    AKL_ADD_CFUN(in, curs_noecho, "NOECHO", "Disable echo on input");
    AKL_ADD_CFUN(in, curs_getch, "GETCH", "Get back a symbol of the pressed key, format: KEY_*");
    AKL_ADD_CFUN(in, curs_printw, "PRINTW", "Print out a given string");
    AKL_ADD_CFUN(in, curs_getmax, "GETMAX", "Get the maximal x, y coordinates of the actual window");
    AKL_ADD_CFUN(in, curs_getxy, "GETXY", "Get the current x, y coordinates of the actual window");
    AKL_ADD_CFUN(in, curs_refresh, "REFRESH", "Update the current window");
    return AKL_LOAD_OK;
}

static int unload_curses_mod(struct akl_state *in)
{
    AKL_REMOVE_CFUN(in, curs_init_scr);
    AKL_REMOVE_CFUN(in, curs_endwin);
    AKL_REMOVE_CFUN(in, curs_noecho);
    AKL_REMOVE_CFUN(in, curs_getch);
    AKL_REMOVE_CFUN(in, curs_printw);
    AKL_REMOVE_CFUN(in, curs_getmax);
    AKL_REMOVE_CFUN(in, curs_getxy);
    AKL_REMOVE_CFUN(in, curs_refresh);
    return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(load_curses_mod, unload_curses_mod
    , "curses", "Terminal-independent method of updating character screens"
    , "Kovacs Akos");
