#include <stdio.h>

#include "common.h"
#include "test_cases.h"



int main(void) {
    int no_failed = 0; 
    Suite *s;                  
    SRunner *sr;                     

    s = suite_create("Master");                   
    sr = srunner_create(s);          

    srunner_add_suite(sr, create_suite_hashmap());
    // srunner_add_suite(sr, create_suite_vm());

    srunner_run_all(sr, CK_VERBOSE);  
    no_failed = srunner_ntests_failed(sr); 
    srunner_free(sr);                      
    return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;  
}