/*
 * This is a simple test example.
 * They just show, how you can create AkLisp tests
 * It will be automatically include "aklisp.h" and link and also link itself
 * to the AkLisp shared library too. (libaklisp_shared.so)
*/

#include <tester.h>

test_res_t dummy_tester_one(void)
{
    return TEST_OK;
}

test_res_t dummy_tester_two(void)
{
    return TEST_FAIL;
}

int main()
{
    /* Nothing really horrible here */
    struct test tests[] = {
        { dummy_tester_one, "The first dummy tester (must be ok)" },
        { dummy_tester_two, "The second dummy tester (should fail)" },
        { NULL, NULL }
    };

    return run_tests("Dummy test", tests);
}
