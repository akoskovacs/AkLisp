#include "aklisp.h"

int main(int argc, const char *argv[])
{
    FILE *fp;
    struct akl_instance *inst;
    struct akl_list *list;
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Cannot open file %s!\n", argv[1]);
            return -1;
        }
    } else {
        fp = stdin;
    }
    inst = akl_new_file_interpreter(fp);

    init_lib(inst);
    list = akl_parse_io(inst);
    akl_eval_program(inst);
    akl_free_instance(inst);
    return 0;
}
