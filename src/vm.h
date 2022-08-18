#ifndef VM_H
#define VM_H

#include "bytecode.h"

typedef struct Frame Frame;
typedef struct Frame {
    ByteIns* caller;
    Frame* caller_frame;
    void** variables;
} Frame;

typedef Vector OperandStack;

void interpret_bc (Program* prog);
void init_global_vars(HashMap* hm, Vector* const_pool, Vector* globals);



#endif
