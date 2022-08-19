#ifndef VM_H
#define VM_H

#include "bytecode.h"


//===================== FRAMES ===============================
typedef struct Frame Frame;
typedef struct Frame {
    void** return_addr;
    Frame* caller_frame;
    void** variables;
    size_t size;
} Frame;

Frame* make_frame(Frame* caller_frame, void** return_addr, size_t size);
void frame_free(Frame* frame);

typedef enum {
    STACK_EMPTY,
    STACK_CONTINUE
} Stack_State;

//===================== VM ===================================

typedef Vector OperandStack;

typedef struct {
    HashMap* globals;
    HashMap* builtins;
    HashMap* labels;
    Vector* constant_pool;
    Frame* current_frame;
    OperandStack* opstack;
    void** ip;

} VM;

VM* make_vm(void);
void vm_free(VM* vm);

//===================== OPCODE FUNCS =========================

void op_lit(VM* vm, int idx);
void op_printf(VM* vm, int format_idx, int nargs);
void op_array(VM* vm);
void op_object(VM* vm);
void op_slot(VM* vm);
void op_setslot(VM* vm);
void op_callslot(VM* vm, int method_idx, int nargs);
void op_call(VM* vm, int method_idx, int nargs);
void op_setlocal(VM* vm, int i);
void op_getlocal(VM* vm, int i);
void op_setglobal(VM* vm);
void op_getglobal(VM* vm);
void op_branch(VM* vm, int method_idx);
void op_goto(VM* vm, int method_idx);
Stack_State op_return(VM* vm);
void op_drop(VM* vm);


//===================== RUN ==================================

void interpret_bc (Program* prog);
void init_global_vars(HashMap* hm, Vector* const_pool, Vector* globals);
void init_labels(HashMap* hm, Vector* const_pool);
void run_vm(VM* vm);


//===================== PRINT ================================

void print_stack(Frame* frame);
void print_opstack(OperandStack* op);
void print_const_pool(Vector* cp);


//===================== PRINT ================================
HashMap* make_builtins(void);

#endif