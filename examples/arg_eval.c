#include <stdio.h>
#include "../src/aklisp.h"

int main(int argc, char **argv) {
    struct akl_state *s;
    struct akl_value *v;
    const char *line = "($ (print \"Hello, world\") (+ 10 44 (* 2 7)))";
    if (argc > 1)
        line = argv[1];

    s = akl_new_string_interpreter("test", line, NULL);
    akl_init_library(s, AKL_LIB_ALL);
    v = akl_exec_eval(s);
    if (AKL_TYPE(v) == AKL_VT_NUMBER) { // Must be, but it's better to check
        printf("%s is %f\n", line,  AKL_GET_NUMBER_VALUE(v));
    } else {
        printf("NaN\n");
    }
    return 0;
}
