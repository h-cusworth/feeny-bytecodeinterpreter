#include <stdio.h>

#include "common.h"
#include "../src/utils.h"
#include "../src/bytecode.h"


START_TEST(test1) {
    size_t n = 20;
    HashMap* hm = make_hashmap(n);

    ck_assert_int_eq(hm->size,n);
    int* val = malloc(sizeof(int));
    *val = 25;
    char* key = "beans";
    hashmap_add(hm, key, val);
    ck_assert_int_eq(*((int*)(hashmap_get(hm, key))), *val);
    free(val);
    hashmap_free(hm);
} END_TEST

START_TEST(test2) {
    size_t n = 5;
    HashMap* hm = make_hashmap(n);
    char* keys[] = {"baz", "bob", "beans", "choc", "fab", "free", "harry", "fence", "goblin"};
    int vals[] = {1,2,3,4,5,6,7,8,9};
    for (int i=0;i<9;i++) {
        hashmap_add(hm, keys[i], &vals[i]);
    }
    for (int i=0;i<9;i++) {
        ck_assert_int_eq(vals[i], *((int*)(hashmap_get(hm, keys[i]))));
    }
    hashmap_free(hm);
}

START_TEST(test3) {
    size_t n = 25;
    HashMap* hm = make_hashmap(n);
    char* key = "noddy";
    int *val = malloc(sizeof(int));
    *val = 44;
    hashmap_add(hm, key, val);
    *val = 55;
    hashmap_add(hm, key, val);
    ck_assert_int_eq(*val, *((int*)(hashmap_get(hm, key))));
    int* val2 = malloc(sizeof(int));
    *val2 = 66;
    hashmap_add(hm, key, val2);
    ck_assert_int_eq(*val2, *((int*)(hashmap_get(hm, key))));
    hashmap_add(hm, key, val);
    ck_assert_int_eq(*val, *((int*)(hashmap_get(hm, key))));
    hashmap_free(hm);
    free(val);
    free(val2);

}

START_TEST(test4) {
    size_t n = 25;
    HashMap* hm = make_hashmap(n);
    char* key = "noddy";
    ValTag tag = METHOD_VAL;
    int name = 3;
    int nargs = 5;
    Vector* code = make_vector();
    MethodValue* val = malloc(sizeof(MethodValue));
    val->tag = tag;
    val->name = name;
    val->nargs = nargs;
    val->code = code;
    hashmap_add(hm, key, val);
    MethodValue* out = hashmap_get(hm, key);
    ck_assert_int_eq(name, out->name); 
    ck_assert_int_eq(nargs, out->nargs); 
    ck_assert_int_eq(tag, out->tag); 
    ck_assert_ptr_eq(code, out->code); 
    hashmap_free(hm);
}


Suite *create_suite_hashmap(void) {
  Suite *s;
  TCase *tc_core;

  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test1);
  tcase_add_test(tc_core, test2);
  tcase_add_test(tc_core, test3);
  tcase_add_test(tc_core, test4);

  s = suite_create("HashMap");

  suite_add_tcase(s, tc_core);

  return s;
}
