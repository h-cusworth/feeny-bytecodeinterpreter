#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#define GLOBALS_SIZE 25


//===================== FRAME ================================

Frame* make_frame(Frame* caller_frame, void** return_addr, size_t size) {
  Frame* frame = malloc(sizeof(Frame));
  frame->return_addr = return_addr;
  frame->caller_frame = caller_frame;
  void** variables =  calloc(size,sizeof(void*));
  frame->variables = variables;
  frame->size = size;
  return frame;
}

void frame_free(Frame* frame) {
  free(frame->variables);
  free(frame);
}

//===================== VM ================================

VM* init_vm(Program* p) {
  VM* vm = malloc(sizeof(VM));
  HashMap* globals = make_hashmap(GLOBALS_SIZE);
  MethodValue* entry_method = vector_get(p->values, p->entry);
  Frame* current_frame = make_frame(NULL, NULL, entry_method->nargs+entry_method->nlocals);
  OperandStack* opstack = make_vector();
  void** ip = &entry_method->code->array[0];
  
  vm->globals = globals;
  vm->constant_pool = p->values;
  vm->current_frame = current_frame;
  vm->opstack = opstack;
  vm->ip = ip;
  init_global_vars(vm->globals, p->values, p->slots);

  return vm;
}

void vm_free(VM* vm) {
  hashmap_free(vm->globals);
  frame_free(vm->current_frame);
  vector_free(vm->opstack);
  free(vm);
}

//===================== OP METHODS ===========================

void op_lit(VM* vm, int idx) {
  Value* value = vector_get(vm->constant_pool, idx);
  vector_add(vm->opstack, value);
}

// To do: formatting
void op_printf(VM* vm, int format_idx, int nargs) {
  char* format = ((StringValue*)(vector_get(vm->constant_pool, format_idx)))->value;
  for (int i=0;i<nargs;i++) {
    Value* operand = vector_pop(vm->opstack);
  }
  printf("%s\n", format);
  Value* null = malloc(sizeof(Value));
  null->tag = NULL_VAL;
  vector_add(vm->opstack, null);
}

void op_array(VM* vm) {
}

void op_object(VM* vm) {
}

void op_slot(VM* vm) {
}

void op_setslot(VM* vm) {
}

// To do: arrays + objects
void op_callslot(VM* vm, int method_idx, int nargs) {
  char* method_name = ((StringValue*)(vector_get(vm->constant_pool, method_idx)))->value;
  MethodValue* method = hashmap_get(vm->globals, method_name);
  Vector* args = make_vector();
  for (int i=0;i<nargs-1;i++) {
    vector_add(args, vector_pop(vm->opstack));
  }
  Value* receiver = vector_pop(vm->opstack);
  
  // switch(receiver->tag) {
  //   case(INT_VAL) {

  //   }
  // }
}

void op_call(VM* vm, int method_idx, int nargs) {
  char* method_name = ((StringValue*)(vector_get(vm->constant_pool, method_idx)))->value;
  MethodValue* method = hashmap_get(vm->globals, method_name);
  Frame* next_frame = make_frame(
    vm->current_frame,
    vm->ip+1,
    method->nargs+method->nlocals);

  for (int i=nargs-1;i>-1;i--) {
    next_frame->variables[i] = vector_pop(vm->opstack);
  }

  vm->current_frame = next_frame;
  vm->ip = &method->code->array[0];
}

void op_setlocal(VM* vm, int i) {
  Value* value = vector_peek(vm->opstack);
  vm->current_frame->variables[i] = value;
}

void op_getlocal(VM* vm, int i) {
  Value* value = vm->current_frame->variables[i];
  vector_add(vm->opstack, value);
}

void op_setglobal(VM* vm) {
}

void op_getglobal(VM* vm) {
}

void op_branch(VM* vm, int method_idx) {
  Value* value = vector_pop(vm->opstack);
  if (value->tag == NULL_VAL) {
    vm->ip++;
    return;
  }
  char* label = ((StringValue*)vector_get(vm->constant_pool, method_idx))->value;
  void** ins = hashmap_get(vm->globals, label);
  vm->ip = ins; 
}

void op_goto(VM* vm, int method_idx) {
  char* label = ((StringValue*)vector_get(vm->constant_pool, method_idx))->value;
  void** ins = hashmap_get(vm->globals, label);
  vm->ip = ins; 
}

Stack_State op_return(VM* vm) {
  if (vm->current_frame->caller_frame != NULL) {
    Frame* dead_frame = vm->current_frame;
    vm->ip = dead_frame->return_addr;
    vm->current_frame = dead_frame->caller_frame;
    free(dead_frame);
    return STACK_CONTINUE;
  }
  return STACK_EMPTY;
}

void op_drop(VM* vm) {
  Value* popped = vector_pop(vm->opstack);
  switch(popped->tag) {
    case (NULL_VAL): free((Value*)popped);
  };
}

//===================== RUN ==================================

void init_global_vars(HashMap* hm, Vector* const_pool, Vector* globals) {
  for (int i=0;i<globals->size;i++) {
    void* val = vector_get(const_pool, (int)vector_get(globals, i));
    int name_idx = ((Value*)val)->tag == SLOT_VAL ? ((SlotValue*)val)->name : ((MethodValue*)val)->name;
    char* name = ((StringValue*)vector_get(const_pool, name_idx))->value;
    hashmap_add(hm, name, val);
  }
  for (int i=0;i<const_pool->size;i++) {
    Value* value = vector_get(const_pool, i);
    if (value->tag == METHOD_VAL) {
      parse_labels(hm, (MethodValue*)value, const_pool);
    }
  }
}

void parse_labels(HashMap* hm, MethodValue* method, Vector* const_pool) {
  for (int i=0;i<method->code->size;i++) {
    ByteIns* ins = vector_get(method->code, i);
    if (ins->tag == LABEL_OP) {
      char* label = ((StringValue*)(vector_get(const_pool, ((LabelIns*)ins)->name)))->value;
      hashmap_add(hm, label, &method->code->array[i]);
    }
  }
  
}

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  print_prog(p);
  printf("\n");

  VM* vm = init_vm(p); 
  run_vm(vm);
  

  vm_free(vm);
}

void run_vm(VM* vm) {
  while (vm->ip != NULL) {
    printf("\n");
    ByteIns* ins = (ByteIns*) *vm->ip;
    switch(ins->tag) {
      case LABEL_OP:{
        LabelIns* i = (LabelIns*)ins;
        printf("label #%d", i->name);
        vm->ip++;
        break;
      }
      case LIT_OP:{
        LitIns* i = (LitIns*)ins;
        printf("   lit #%d", i->idx);
        op_lit(vm, i->idx);
        vm->ip++;
        break;
      }
      case PRINTF_OP:{
        PrintfIns* i = (PrintfIns*)ins;
        printf("   printf #%d %d\n", i->format, i->arity);
        op_printf(vm, i->format, i->arity);
        vm->ip++;
        break;
      }
      case ARRAY_OP:{
        printf("   array");
        vm->ip++;
        break;
      }
      case OBJECT_OP:{
        ObjectIns* i = (ObjectIns*)ins;
        printf("   object #%d", i->class);
        vm->ip++;
        break;
      }
      case SLOT_OP:{
        SlotIns* i = (SlotIns*)ins;
        printf("   slot #%d", i->name);
        vm->ip++;
        break;
      }
      case SET_SLOT_OP:{
        SetSlotIns* i = (SetSlotIns*)ins;
        printf("   set-slot #%d", i->name);
        vm->ip++;
        break;
      }
      case CALL_SLOT_OP:{
        CallSlotIns* i = (CallSlotIns*)ins;
        printf("   call-slot #%d %d", i->name, i->arity);
        op_callslot(vm, i->name, i->arity);
        vm->ip++;
        break;
      }
      case CALL_OP:{
        CallIns* i = (CallIns*)ins;
        printf("   call #%d %d\n", i->name, i->arity);
        op_call(vm, i->name, i->arity);
        break;
      }
      case SET_LOCAL_OP:{
        SetLocalIns* i = (SetLocalIns*)ins;
        printf("   set local %d", i->idx);
        op_setlocal(vm, i->idx);
        // print_stack(vm->current_frame);
        vm->ip++;
        break;
      }
      case GET_LOCAL_OP:{
        GetLocalIns* i = (GetLocalIns*)ins;
        printf("   get local %d", i->idx);
        op_getlocal(vm, i->idx);
        // print_opstack(vm->opstack);
        vm->ip++;
        break;
      }
      case SET_GLOBAL_OP:{
        SetGlobalIns* i = (SetGlobalIns*)ins;
        printf("   set global #%d", i->name);
        vm->ip++;
        break;
      }
      case GET_GLOBAL_OP:{
        GetGlobalIns* i = (GetGlobalIns*)ins;
        printf("   get global #%d", i->name);
        vm->ip++;
        break;
      }
      case BRANCH_OP:{
        BranchIns* i = (BranchIns*)ins;
        printf("   branch #%d", i->name);
        op_branch(vm, i->name);
        break;
      }
      case GOTO_OP:{
        GotoIns* i = (GotoIns*)ins;
        printf("   goto #%d", i->name);
        op_goto(vm, i->name);
        break;
      }
      case RETURN_OP:{
        printf("   return");
        if (op_return(vm) == STACK_EMPTY) vm->ip = NULL;
        break;
      }
      case DROP_OP:{
        printf("   drop");
        op_drop(vm);
        vm->ip++;
        break;
      }
      default:{
        printf("Unknown instruction with tag: %u\n", ins->tag);
        exit(-1);
      }
    }
    printf("\n");
  }
}

//===================== PRINT ================================

void print_frame(Frame* frame) {
  if (frame == STACK_EMPTY) return;
  printf("----------\n");
  printf("return addr: ");
  if (frame->caller_frame != NULL) print_ins(*frame->return_addr);
  else printf("   null");
  printf("\nvariables: \n");
  for (int i=0;i<frame->size;i++) {
    if (frame->variables[i]) {
      printf("    ");
      print_value((Value*)frame->variables[i]);
      printf("\n");
    }
  }
  print_frame(frame->caller_frame);
}

void print_stack(Frame* frame) {
  printf("\n---- CURRENT STACK-----\n");
  printf("\nTOP\n");
  print_frame(frame);
  printf("----------\n");
  printf("FLOOR\n");
  printf("\n---------------------\n");
}

void print_value_vector(Vector* v) {
  for (int i=0;i<v->size;i++) {
    printf("  %i: ", i);
    print_value(vector_get(v, i));
    printf(",\n");
  }
}

void print_opstack(OperandStack* op) {
  printf("\n---- OPSTACK -----\n");
  print_value_vector(op);
  printf("\n");
}

void print_const_pool(Vector* cp) {
  printf("\n---- CONST POOL -----\n");
  print_value_vector(cp);
  printf("\n");
}







