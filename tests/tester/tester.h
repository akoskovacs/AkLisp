#ifndef TEST_H
#define TEST_H

#define USE_COLORS 1
#include <aklisp.h>

typedef enum {
   TEST_OK, TEST_FAIL
} test_res_t;

typedef test_res_t (*test_fn_t)(void);

struct test {
    test_fn_t  test_fn;
    const char *test_desc;
};

int run_tests(const char *, struct test *);

#endif // TEST_H
