#include <tester.h>

struct akl_state state;
struct akl_list *list = NULL;
int nums[] = { -13, 22, 33, 11, 44, 122, 42, 112, 331, 23 };
#define NR_NUMS (sizeof(nums)/sizeof(int))
int ind42 = 6;

test_res_t list_create(void)
{
    /* The vector must grow */
    list = akl_new_list(&state);
    return list ? TEST_OK : TEST_FAIL;
}

test_res_t list_append(void)
{
    int i;
    for (i = 0; i < NR_NUMS; i++) {
       akl_list_append(&state, list, (void *)nums+i);
       if (AKL_LIST_LAST(list)->le_data != (void *)nums+i)
           return TEST_FAIL;
    }
    return TEST_OK;
}

test_res_t list_foreach(void)
{
    int i = 0;
    struct akl_list_entry *ent;
    AKL_LIST_FOREACH(ent, list) {
        if (ent->le_data != (void *)nums+i)
            return TEST_FAIL;
        i++;
    }
    return TEST_OK;
}

test_res_t list_size(void)
{
    return akl_list_count(list) == NR_NUMS;
}

test_res_t list_index(void)
{
    int i;
    bool_t right = TRUE;
    right = akl_list_index(list, 5) == (void *)nums+5;
    right = right && (akl_list_index(list, -2) == (void *)nums+8);
    right = right && (akl_list_index(list, 0) == (void *)nums);
    return right;
}

int number_finder(void *p1, void *p2)
{
    int *n1 = (int *)p1;
    int *n2 = (int *)p2;
    return *n1 == *n2 ? 0 : 1;
}

test_res_t list_find(void)
{
    int f = 42;
    int ind = 0;
    akl_list_find(list, number_finder, &f, &ind);
    return ind == ind42;
}

test_res_t list_first(void)
{
    int *n = AKL_LIST_FIRST(list)->le_data;
    return *n == nums[0];
}

test_res_t list_insert_head(void)
{
    int n = 99;
    akl_list_insert_head(&state, list, (void *)&n);
    return *((int *)AKL_LIST_FIRST(list)->le_data) == 99;
}


test_res_t list_remove(void)
{
    struct akl_list_entry *h = AKL_LIST_FIRST(list);
    akl_list_remove_entry(list, h);
    return AKL_LIST_FIRST(list)->le_data == (void *)nums+0;
}

int main()
{
    akl_init_state(&state, NULL);
    struct test vtests[] = {
        { list_create, "akl_new_list() can create a list" },
        { list_append, "akl_list_append() can add elements to a list" },
        { list_foreach, "AKL_LIST_FOREACH() can iterate through the elements" },
        { list_size, "akl_list_count() gives back the size of the list" },
        { list_index, "akl_list_index() can get back different elements" },
        { list_first, "AKL_LIST_FIRST() can get the first element" },
        { list_insert_head, "akl_insert_head() can insert a new first elemenet" },
        { list_remove, "akl_list_remove_entry() can remove arbitrary elements" },
        { NULL, NULL }
    };
    return run_tests("List test", vtests);
}
