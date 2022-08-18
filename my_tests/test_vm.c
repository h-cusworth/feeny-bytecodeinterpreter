#include <stdio.h>

#include "common.h"
#include "../src/vm.h"



// START_TEST(test1) {
//     HashMap* hm = malloc(sizeof(HashMap));
//     Vector* constant_pool = make_vector();
//     Vector* globals = make_vector();
//     char* name = "main";
//     char* name2 = "foo";
//     SlotValue* slot = malloc(sizeof(SlotValue));
//     slot->name = 0; 
//     MethodValue* method = malloc(sizeof(MethodValue));
//     method->name = 2;
//     int* idx1 = malloc(sizeof(int));
//     *idx1 = 1;
//     int* idx2 = malloc(sizeof(int));
//     *idx2 = 3;
//     vector_add(constant_pool, name);
//     vector_add(constant_pool, slot);
//     vector_add(constant_pool, name2);
//     vector_add(constant_pool, method);
//     vector_add(globals, idx1);
//     vector_add(globals, idx2);
//     init_global_vars(hm, constant_pool, globals);
//     ck_assert_ptr_eq(&slot, hashmap_get(hm, name));
//     ck_assert_ptr_eq(&method, hashmap_get(hm, name2));


// } END_TEST

Suite *create_suite_vm(void) {
  Suite *s;
  TCase *tc_core;

  tc_core = tcase_create("Core");

//   tcase_add_test(tc_core, test1);

  s = suite_create("VM");

  suite_add_tcase(s, tc_core);

  return s;
}