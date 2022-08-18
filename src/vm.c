#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#define GLOBALS_SIZE 25


Frame* make_frame(Frame* caller_frame, ByteIns* caller, size_t size) {
  Frame* frame = malloc(sizeof(Frame));
  frame->caller = caller;
  frame->caller_frame = caller_frame;
  void** variables =  malloc(sizeof(void*)*size);
  frame->variables = variables;
  return frame;
}

void frame_free(Frame* frame) {
  free(frame->variables);
  free(frame);
}

void init_global_vars(HashMap* hm, Vector* const_pool, Vector* globals) {
  for (int i=0;i<globals->size;i++) {
    void* val = vector_get(const_pool, (int)vector_get(globals, i));
    int name_idx = ((Value*)val)->tag == SLOT_VAL ? ((SlotValue*)val)->name : ((MethodValue*)val)->name;
    char* name = ((StringValue*)vector_get(const_pool, name_idx))->value;
    hashmap_add(hm, name, val);
  }
}


void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  print_prog(p);
  printf("\n");

  HashMap* globals = make_hashmap(GLOBALS_SIZE);
  init_global_vars(globals, p->values, p->slots);
  MethodValue* entry_method = vector_get(p->values, p->entry);
  void* ip = vector_get(entry_method->code, 0);
  Frame* current_frame = make_frame(NULL, NULL, entry_method->nargs+entry_method->nlocals);
  OperandStack* opstack = make_vector();

  while (ip != NULL) {
    printf("\n");
    ByteIns* ins = ip;
    switch(ins->tag) {
      case LABEL_OP:{
        LabelIns* i = (LabelIns*)ins;
        printf("label #%d", i->name);
        break;
      }
      case LIT_OP:{
        LitIns* i = (LitIns*)ins;
        printf("   lit #%d", i->idx);
        break;
      }
      case PRINTF_OP:{
        PrintfIns* i = (PrintfIns*)ins;
        printf("   printf #%d %d", i->format, i->arity);
        break;
      }
      case ARRAY_OP:{
        printf("   array");
        break;
      }
      case OBJECT_OP:{
        ObjectIns* i = (ObjectIns*)ins;
        printf("   object #%d", i->class);
        break;
      }
      case SLOT_OP:{
        SlotIns* i = (SlotIns*)ins;
        printf("   slot #%d", i->name);
        break;
      }
      case SET_SLOT_OP:{
        SetSlotIns* i = (SetSlotIns*)ins;
        printf("   set-slot #%d", i->name);
        break;
      }
      case CALL_SLOT_OP:{
        CallSlotIns* i = (CallSlotIns*)ins;
        printf("   call-slot #%d %d", i->name, i->arity);
        break;
      }
      case CALL_OP:{
        CallIns* i = (CallIns*)ins;
        printf("   call #%d %d", i->name, i->arity);
        break;
      }
      case SET_LOCAL_OP:{
        SetLocalIns* i = (SetLocalIns*)ins;
        printf("   set local %d", i->idx);
        break;
      }
      case GET_LOCAL_OP:{
        GetLocalIns* i = (GetLocalIns*)ins;
        printf("   get local %d", i->idx);
        break;
      }
      case SET_GLOBAL_OP:{
        SetGlobalIns* i = (SetGlobalIns*)ins;
        printf("   set global #%d", i->name);
        break;
      }
      case GET_GLOBAL_OP:{
        GetGlobalIns* i = (GetGlobalIns*)ins;
        printf("   get global #%d", i->name);
        break;
      }
      case BRANCH_OP:{
        BranchIns* i = (BranchIns*)ins;
        printf("   branch #%d", i->name);
        break;
      }
      case GOTO_OP:{
        GotoIns* i = (GotoIns*)ins;
        printf("   goto #%d", i->name);
        break;
      }
      case RETURN_OP:{
        printf("   return");
        ip = NULL;
        break;
      }
      case DROP_OP:{
        printf("   drop");
        break;
      }
      default:{
        printf("Unknown instruction with tag: %u\n", ins->tag);
        exit(-1);
      }
    }
    ip += sizeof(void*);
    printf("\n");
  }

  hashmap_free(globals);
  vector_free(opstack);
  frame_free(current_frame);
}

// void execute_method(MethodValue* method, Frame* frame, ByteIns* ip, OperandStack opstack, HashMap* globals) {
//   Vector* code = method->code; 
//   for (int i=0;i<ode->size;i++) {
//       ip = vector_get()
//       case (GOTO_OP):
//         printf("THIS IS A GOTO OPCODE!!!!!\n");
//       default:
//         print_ins(IP);
//   }


// }







