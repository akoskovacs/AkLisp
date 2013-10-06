#include <tester.h>

struct akl_state state;
struct akl_vector *vec = NULL;
int nums[] = { -13, 22, 33, 11, 44, 122, 42, 112, 331, 23 };
int ind42 = 6;

test_res_t vector_create(void)
{
    /* The vector must grow */
    vec = akl_new_vector(&state, 8, sizeof(int));
    return vec ? TEST_OK : TEST_FAIL;
}

test_res_t vector_push(void)
{
    int n;
    int *np;
    int i;
    for (i = 0; i < sizeof(nums)/sizeof(int); i++) {
       n = akl_vector_push(vec, &nums[i]);
       if (n != i) 
           return TEST_FAIL;
    }
    np = akl_vector_at(vec, 4);
    return *np == 44;
}

test_res_t vector_size(void)
{
    if (akl_vector_count(vec) != sizeof(nums)/sizeof(int))
        return TEST_FAIL;
    return TEST_OK;
}

test_res_t vector_at(void)
{
    int i;
    int *n;
    for (i = 0; i < akl_vector_count(vec); i++) {
        n = (int *)akl_vector_at(vec, i);
        if (*n != nums[i])
            return TEST_FAIL;
    }
    return TEST_OK;
}

int number_finder(void *p1, void *p2)
{
    int *n1 = (int *)p1;
    int *n2 = (int *)p2;
    return *n1 == *n2 ? 0 : 1;
}

test_res_t vector_find(void)
{
    int f = 42;
    unsigned int ind = 0;
    akl_vector_find(vec, number_finder, (void *)&f, &ind);
    return ind == ind42;
}

test_res_t vector_first(void)
{
    int *n = (int *)akl_vector_first(vec);
    return *n == nums[0];
}

test_res_t vector_reserve_test(int num)
{
    int *p;
    int *n = akl_vector_reserve(vec);
    *n=num;
    p = (int *)akl_vector_at(vec, akl_vector_count(vec)-1);
    return *p == num;
}

test_res_t vector_reserve(void)
{
    return vector_reserve_test(1994) && vector_reserve_test(10);
}

test_res_t vector_pop(void)
{
    int i;
    int *n;
    akl_vector_pop(vec);
    akl_vector_pop(vec);
    for (i = akl_vector_count(vec)-1; i >= 0; i--) {
        n = (int *)akl_vector_pop(vec);        
        if (*n != nums[i])
            return TEST_FAIL;
    }
    return TEST_OK;
}

int main()
{
    akl_init_state(&state);
    struct test vtests[] = {
        { vector_create, "Can create vector with akl_vector_new()" },
        { vector_push, "Can add elements to a vector with akl_vector_push()" },
        { vector_at, "akl_vector_at() gives back the right elements" },
        { vector_first, "akl_vector_first() gives back the first element" },
        { vector_find, "akl_vector_find() can find an element" },
        { vector_size, "akl_vector_count() knows the vector size" },
        { vector_reserve, "akl_vector_reserve() can reserve room for the next element" },
        { vector_pop, "akl_vector_pop() can pop back the elements" },
        { NULL, NULL }
    };
    return run_tests("Vector test", vtests);
}
