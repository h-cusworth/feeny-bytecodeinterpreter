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
  Value** variables =  calloc(size,sizeof(Value*));
  frame->variables = variables;
  frame->size = size;
  return frame;
}

void frame_free(Frame* frame) {
  free(frame->variables);
  free(frame);
}

//===================== VM ================================

#define LABELS_SIZE 10

VM* init_vm(Program* p) {
  VM* vm = malloc(sizeof(VM));
  HashMap* globals = make_hashmap(GLOBALS_SIZE);
  HashMap* builtins = make_builtins();
  HashMap* labels = make_hashmap(LABELS_SIZE);
  MethodValue* entry_method = vector_get(p->values, p->entry);
  Frame* current_frame = make_frame(NULL, NULL, entry_method->nargs+entry_method->nlocals);
  OperandStack* opstack = make_vector();
  void** ip = &entry_method->code->array[0];
  
  vm->globals = globals;
  vm->builtins = builtins;
  vm->labels = labels;
  vm->constant_pool = p->values;
  vm->current_frame = current_frame;
  vm->opstack = opstack;
  vm->ip = ip;
  init_global_vars(vm->globals, p->values, p->slots);
  init_labels(vm->labels, vm->constant_pool);

  return vm;
}

void vm_free(VM* vm) {
  hashmap_free(vm->globals);
  hashmap_free(vm->builtins);
  hashmap_free(vm->labels);
  frame_free(vm->current_frame);
  vector_free(vm->opstack);
  free(vm);
}

//===================== OPCODE FUNCS =========================


void op_lit(VM* vm, int idx) {
  Value* value = vector_get(vm->constant_pool, idx);
  vector_add(vm->opstack, value);
}

void op_printf(VM* vm, int format_idx, int nargs) {
  char* format = ((StringValue*)(vector_get(vm->constant_pool, format_idx)))->value;
  Vector* args = make_vector();
  for (int i=0;i<nargs;i++) vector_add(args,vector_pop(vm->opstack));
  for (int i=0;i<strlen(format);i++) {
    if (format[i] == '~') {
      Value* val = vector_pop(args);
      switch (val->tag) {
        case (INT_VAL):
          printf("%i", ((IntValue*)val)->value);
      }
    }
    else printf("%c", format[i]);
  }
  Value* null = malloc(sizeof(Value));
  null->tag = NULL_VAL;
  vector_add(vm->opstack, null);
  vector_free(args);
}

void op_array(VM* vm) {
  int init = ((IntValue*)(vector_pop(vm->opstack)))->value;
  size_t len = ((IntValue*)(vector_pop(vm->opstack)))->value;
  int* arr = malloc(sizeof(int)*len);
  memset(arr, init, len);
  ArrayValue* array = malloc(sizeof(ArrayValue));
  array->tag = ARRAY_VAL;
  array->value = arr;
  array->length = len;
  vector_add(vm->opstack, array);
}

void op_object(VM* vm, int class_idx) {
  ClassValue* class = vector_get(vm->constant_pool, class_idx);
  size_t slots_count = 0;
  for (int i=0;i<class->slots->size;i++) {
    Value* v = vector_get(vm->constant_pool, (int)vector_get(class->slots, i));
    if (v->tag == SLOT_VAL) slots_count++;
  }
  Vector* slots = make_vector();
  vector_set_length(slots, slots_count, NULL);
  int j = 0;
  for (int i=0;i<class->slots->size;i++) {
    Value* v = vector_get(vm->constant_pool, (int)vector_get(class->slots, i));
    if (v->tag == SLOT_VAL) {
      Value* val = vector_pop(vm->opstack);
      vector_set(slots, slots_count - j - 1, val);
      j++;
    }
  }
  vector_add(slots, vector_pop(vm->opstack));
  ObjectValue* obj = malloc(sizeof(ObjectValue));
  obj->tag = OBJ_VAL;
  obj->slots = slots;
  obj->class = class;
  vector_add(vm->opstack, obj);
}

Value* get_object_slot_value(Vector* constant_pool, ObjectValue* obj, int constpool_idx) {
  if (obj->tag == NULL_VAL) {
    printf("Unknown slot called.");
    exit(-1);
  }

  int obj_idx = 0;
  for (int i=0;i<obj->class->slots->size;i++) {
    SlotValue* slot = vector_get(constant_pool, (int)vector_get(obj->class->slots, i));
    if (slot->tag == SLOT_VAL && slot->name == constpool_idx) return vector_get(obj->slots, obj_idx);
    if (slot->tag == SLOT_VAL) obj_idx++;
  }

  get_object_slot_value(constant_pool, vector_peek(obj->slots), constpool_idx);
}

void op_getslot(VM* vm, int idx) {
  ObjectValue* obj = vector_pop(vm->opstack);
  Value* value = get_object_slot_value(vm->constant_pool, obj, idx);
  vector_add(vm->opstack, value);
}

void set_object_slot_value(Vector* constant_pool, ObjectValue* obj, int constpool_idx, Value* value) {
  if (obj->tag == NULL_VAL) {
    printf("Unknown slot called.");
    exit(-1);
  }

  int obj_idx = 0;
  for (int i=0;i<obj->class->slots->size;i++) {
    SlotValue* slot = vector_get(constant_pool, (int)vector_get(obj->class->slots, i));
    if (slot->tag == SLOT_VAL && constpool_idx == slot->name) {
      vector_set(obj->slots, obj_idx, value);
      return;
    }
    if (slot->tag == SLOT_VAL) obj_idx++;
  }

  set_object_slot_value(constant_pool, vector_peek(obj->slots), constpool_idx, value);
}
void op_setslot(VM* vm, int idx) {
  Value* value = vector_pop(vm->opstack);
  ObjectValue* obj = vector_pop(vm->opstack);
  set_object_slot_value(vm->constant_pool, obj, idx, value);
  vector_add(vm->opstack, value);
}

MethodValue* get_method(Vector* constant_pool, ObjectValue* obj, int constpool_idx) {
  if (obj->tag == NULL_VAL) {
    printf("Unknown method called.");
    exit(-1);
  }

  for (int i=0;i<obj->class->slots->size;i++) {
    MethodValue* method = vector_get(constant_pool, (int)vector_get(obj->class->slots, i));
    if (method->tag == METHOD_VAL && method->name == constpool_idx) return method;
  }
  get_method(constant_pool, vector_peek(obj->slots), constpool_idx);
}

void op_callslot(VM* vm, int method_idx, int nargs) {
  char* method_name = ((StringValue*)(vector_get(vm->constant_pool, method_idx)))->value;
  Value** args = malloc(sizeof(Value*)*nargs);
  for (int i=0;i<nargs;i++) {
    args[i] = vector_pop(vm->opstack);
  }
  Value* receiver = args[nargs-1];
  if (method_idx == 5) {
  }
  if (receiver->tag == OBJ_VAL) {
    MethodValue* method = get_method(vm->constant_pool, (ObjectValue*)receiver, method_idx);
    Frame* next_frame = make_frame(
      vm->current_frame,
      vm->ip+1,
      method->nargs+method->nlocals);
    next_frame->variables[0] = receiver;
    for (int i=nargs-2;i>-1;i--) {
      next_frame->variables[nargs - i - 1] = args[i];
    }
    vm->current_frame = next_frame;
    vm->ip = &method->code->array[0];
  }
  else {
    Value* (*func)(Value**) = hashmap_get(vm->builtins, method_name);
    Value* return_value = (*func)(args);
    vector_add(vm->opstack, return_value);
    vm->ip++;
  }
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

void op_setlocal(VM* vm, int idx) {
  Value* value = vector_peek(vm->opstack);
  vm->current_frame->variables[idx] = value;
}

void op_getlocal(VM* vm, int idx) {
  Value* value = vm->current_frame->variables[idx];
  vector_add(vm->opstack, value);
}

void op_setglobal(VM* vm, int idx) {
  char* key = ((StringValue*)(vector_get(vm->constant_pool, idx)))->value;
  hashmap_add(vm->globals, key, vector_peek(vm->opstack));
}

void op_getglobal(VM* vm, int idx) {
  char* key = ((StringValue*)(vector_get(vm->constant_pool, idx)))->value;
  vector_add(vm->opstack, hashmap_get(vm->globals, key));
}

void op_branch(VM* vm, int method_idx) {
  Value* value = vector_pop(vm->opstack);
  if (value->tag == NULL_VAL) {
    vm->ip++;
    return;
  }
  char* label = ((StringValue*)vector_get(vm->constant_pool, method_idx))->value;
  void** ins = hashmap_get(vm->labels, label);
  vm->ip = ins; 
}

void op_goto(VM* vm, int method_idx) {
  char* label = ((StringValue*)vector_get(vm->constant_pool, method_idx))->value;
  void** ins = hashmap_get(vm->labels, label);
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
  vector_pop(vm->opstack);
}

//===================== RUN ==================================

void init_global_vars(HashMap* hm, Vector* const_pool, Vector* globals) {
  for (int i=0;i<globals->size;i++) {
    void* val = vector_get(const_pool, (int)vector_get(globals, i));
    int name_idx = ((Value*)val)->tag == SLOT_VAL ? ((SlotValue*)val)->name : ((MethodValue*)val)->name;
    char* name = ((StringValue*)vector_get(const_pool, name_idx))->value;
    hashmap_add(hm, name, val);
  }
}

void init_labels(HashMap* hm, Vector* const_pool) {
  for (int i=0;i<const_pool->size;i++) {
    Value* value = vector_get(const_pool, i);
    if (value->tag == METHOD_VAL) {
      MethodValue* method = (MethodValue*)value;
      for (int i=0;i<method->code->size;i++) {
        ByteIns* ins = vector_get(method->code, i);
        if (ins->tag == LABEL_OP) {
          char* label = ((StringValue*)(vector_get(const_pool, ((LabelIns*)ins)->name)))->value;
          hashmap_add(hm, label, &method->code->array[i]);
        }
      }
    }
  }
}

void interpret_bc (Program* p) {
#ifdef DEBUG
  printf("Interpreting Bytecode Program:\n");
  print_prog(p);
  printf("\n");
#endif

  VM* vm = init_vm(p); 
#ifdef DEBUG
  hashmap_print(vm->globals);
  hashmap_print(vm->labels);
  hashmap_print(vm->builtins);
#endif 
  run_vm(vm);
  

  vm_free(vm);
}

void run_vm(VM* vm) {
  while (vm->ip != NULL) {
    ByteIns* ins = (ByteIns*) *vm->ip;
    switch(ins->tag) {
      case LABEL_OP:{
        LabelIns* i = (LabelIns*)ins;
#ifdef DEBUG
        printf("label #%d", i->name);
#endif
        vm->ip++;
        break;
      }
      case LIT_OP:{
        LitIns* i = (LitIns*)ins;
#ifdef DEBUG
        printf("   lit #%d", i->idx);
#endif
        op_lit(vm, i->idx);
        vm->ip++;
        break;
      }
      case PRINTF_OP:{
        PrintfIns* i = (PrintfIns*)ins;
#ifdef DEBUG
        printf("   printf #%d %d\n", i->format, i->arity);
#endif
        op_printf(vm, i->format, i->arity);
        vm->ip++;
        break;
      }
      case ARRAY_OP:{
#ifdef DEBUG
        printf("   array");
#endif
        op_array(vm);
        vm->ip++;
        break;
      }
      case OBJECT_OP:{
        ObjectIns* i = (ObjectIns*)ins;
#ifdef DEBUG
        printf("   object #%d", i->class);
#endif
        op_object(vm, i->class);
        vm->ip++;
        break;
      }
      case SLOT_OP:{
        SlotIns* i = (SlotIns*)ins;
#ifdef DEBUG
        printf("   slot #%d", i->name);
#endif
        op_getslot(vm, i->name);
        vm->ip++;
        break;
      }
      case SET_SLOT_OP:{
        SetSlotIns* i = (SetSlotIns*)ins;
#ifdef DEBUG
        printf("   set-slot #%d", i->name);
#endif
        op_setslot(vm, i->name);
        vm->ip++;
        break;
      }
      case CALL_SLOT_OP:{
        CallSlotIns* i = (CallSlotIns*)ins;
#ifdef DEBUG
        printf("   call-slot #%d %d", i->name, i->arity);
#endif
        op_callslot(vm, i->name, i->arity);
        break;
      }
      case CALL_OP:{
        CallIns* i = (CallIns*)ins;
#ifdef DEBUG
        printf("   call #%d %d\n", i->name, i->arity);
#endif
        op_call(vm, i->name, i->arity);
        break;
      }
      case SET_LOCAL_OP:{
        SetLocalIns* i = (SetLocalIns*)ins;
#ifdef DEBUG
        printf("   set local %d", i->idx);
#endif
        op_setlocal(vm, i->idx);
        vm->ip++;
        break;
      }
      case GET_LOCAL_OP:{
        GetLocalIns* i = (GetLocalIns*)ins;
#ifdef DEBUG
        printf("   get local %d", i->idx);
#endif
        op_getlocal(vm, i->idx);
        vm->ip++;
        break;
      }
      case SET_GLOBAL_OP:{
        SetGlobalIns* i = (SetGlobalIns*)ins;
#ifdef DEBUG
        printf("   set global #%d", i->name);
#endif
        op_setglobal(vm, i->name);
        vm->ip++;
        break;
      }
      case GET_GLOBAL_OP:{
        GetGlobalIns* i = (GetGlobalIns*)ins;
#ifdef DEBUG
        printf("   get global #%d", i->name);
#endif
        op_getglobal(vm, i->name);
        vm->ip++;
        break;
      }
      case BRANCH_OP:{
        BranchIns* i = (BranchIns*)ins;
#ifdef DEBUG
        printf("   branch #%d", i->name);
#endif
        op_branch(vm, i->name);
        break;
      }
      case GOTO_OP:{
        GotoIns* i = (GotoIns*)ins;
#ifdef DEBUG
        printf("   goto #%d", i->name);
#endif
        op_goto(vm, i->name);
        break;
      }
      case RETURN_OP:{
#ifdef DEBUG
        printf("   return");
#endif
        if (op_return(vm) == STACK_EMPTY) vm->ip = NULL;
        break;
      }
      case DROP_OP:{
#ifdef DEBUG
        printf("   drop");
#endif
        op_drop(vm);
        vm->ip++;
        break;
      }
      default:{
        printf("Unknown instruction with tag: %u\n", ins->tag);
        exit(-1);
      }
    }
#ifdef DEBUG
    printf("\n");
#endif
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
      print_value(frame->variables[i]);
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

//===================== BUILTINS ================================
#define BUILTINS_SIZE 10

Value* feeny_add(Value** args);
Value* feeny_sub(Value** args);
Value* feeny_mul(Value** args);
Value* feeny_div(Value** args);
Value* feeny_mod(Value** args);
Value* feeny_lt(Value** args);
Value* feeny_gt(Value** args);
Value* feeny_le(Value** args);
Value* feeny_ge(Value** args);
Value* feeny_eq(Value** args);
Value* feeny_length(Value** args);
Value* feeny_set(Value** args);
Value* feeny_get(Value** args);

HashMap* make_builtins(void) {
  HashMap* builtins = make_hashmap(BUILTINS_SIZE);
  hashmap_add(builtins, "add", &feeny_add);
  hashmap_add(builtins, "sub", &feeny_sub);
  hashmap_add(builtins, "mul", &feeny_mul);
  hashmap_add(builtins, "div", &feeny_div);
  hashmap_add(builtins, "mod", &feeny_mod);
  hashmap_add(builtins, "lt", &feeny_lt);
  hashmap_add(builtins, "gt", &feeny_gt);
  hashmap_add(builtins, "le", &feeny_le);
  hashmap_add(builtins, "ge", &feeny_ge);
  hashmap_add(builtins, "eq", &feeny_eq);
  hashmap_add(builtins, "length", &feeny_length);
  hashmap_add(builtins, "set", &feeny_set);
  hashmap_add(builtins, "get", &feeny_get);
  return builtins;
}

Value* make_null_val() {
  Value* v = malloc(sizeof(Value));
  v->tag = NULL_VAL;
  return v;
}

Value* make_int_val(int i) {
  IntValue* v = malloc(sizeof(IntValue));
  v->tag = INT_VAL;
  v->value = i;
  return (Value*)v;
}

Value* feeny_add(Value** args) {
  return make_int_val(((IntValue*)(args[1]))->value + ((IntValue*)(args[0]))->value);
}

Value* feeny_sub(Value** args) {
  return make_int_val(((IntValue*)(args[1]))->value - ((IntValue*)(args[0]))->value);
}

Value* feeny_mul(Value** args) {
  return make_int_val(((IntValue*)(args[1]))->value * ((IntValue*)(args[0]))->value);
}

Value* feeny_div(Value** args) {
  return make_int_val(((IntValue*)(args[1]))->value / ((IntValue*)(args[0]))->value);
}

Value* feeny_mod(Value** args) {
  return make_int_val(((IntValue*)(args[1]))->value % ((IntValue*)(args[0]))->value);
}

Value* feeny_eq(Value** args) {
  if (((IntValue*)(args[1]))->value == ((IntValue*)(args[0]))->value) return make_int_val(0);
  else return make_null_val(); 
}

Value* feeny_lt(Value** args) {
  if (((IntValue*)(args[1]))->value < ((IntValue*)(args[0]))->value) return make_int_val(0);
  else return make_null_val(); 
}

Value* feeny_gt(Value** args) {
  if (((IntValue*)(args[1]))->value > ((IntValue*)(args[0]))->value) return make_int_val(0);
  else return make_null_val(); 
}

Value* feeny_le(Value** args) {
  if (((IntValue*)(args[1]))->value <= ((IntValue*)(args[0]))->value) return make_int_val(0);
  else return make_null_val(); 
}

Value* feeny_ge(Value** args) {
  if (((IntValue*)(args[1]))->value >= ((IntValue*)(args[0]))->value) return make_int_val(0);
  else return make_null_val(); 
}

Value* feeny_length(Value** args) {
  ArrayValue* array = (ArrayValue*)args[0];
  return make_int_val(array->length);
}

Value* feeny_set(Value** args) {
  int value = ((IntValue*)(args[0]))->value;
  int idx = ((IntValue*)(args[1]))->value;
  ArrayValue* array = (ArrayValue*)args[2];
  array->value[idx] = value;
  return make_null_val();
}

Value* feeny_get(Value** args) {
  int idx = ((IntValue*)(args[0]))->value;
  ArrayValue* array = (ArrayValue*)args[1];
  return make_int_val(array->value[idx]);
  return make_int_val(2);
}
