#include "tester.h"

const char fail_str[] = "FAILED";
const char fail_str_color[] = AKL_RED "FAILED" AKL_END_COLOR_MARK ;
const char ok_str[] = "OK";
const char ok_str_color[] = AKL_GREEN "OK" AKL_END_COLOR_MARK ;
const char dot_str[] = ".";
size_t term_width = 80;


int get_term_width(void)
{
    int cols = 80;
#ifdef USE_TPUT
    FILE *tput = popen("tput cols", "r");
    if (!tput)
	goto retcol;

    fscanf(tput, "%d", &cols);
#endif

retcol:
    return cols;
}

void print_status(const char *desc, test_res_t res)
{
    const char *res_str = NULL;
    size_t res_width = 0;
    size_t desc_width = 0;
    int i;

    if (desc == NULL)
        desc = "(Test description not available)";
    
    res_str = (res == TEST_OK) ? ok_str_color : fail_str_color;
    res_width = (res == TEST_OK) ? sizeof(ok_str) : sizeof(fail_str);
    desc_width = printf("%s ", desc);
    for (i = 0; i < term_width-desc_width-res_width-2; i++)
        printf(dot_str);

    printf(" [%s]\n", res_str);
}

int run_tests(const char *name, struct test *tests)
{
    int fail_cnt = 0;
    int ok_cnt = 0;
    test_res_t res;
    term_width = get_term_width();

    while (tests->test_fn) {
        res = tests->test_fn();
        if (res == TEST_OK)
            ok_cnt++;
        else
            fail_cnt++;

        print_status(tests->test_desc, res);
        tests++;
    }

    printf(AKL_YELLOW "Test set \'%s\' is completed. " AKL_END_COLOR_MARK
                    "[%d test, " AKL_GREEN "%d ok" AKL_END_COLOR_MARK ", " 
                    AKL_RED "%d fail" AKL_END_COLOR_MARK "]\n"
        , name, ok_cnt+fail_cnt, ok_cnt, fail_cnt);

    return (fail_cnt > 0) ? 1 : 0;
}
