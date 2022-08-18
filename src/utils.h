#ifndef UTILS_H
#define UTILS_H


int max (int a, int b);
int min (int a, int b);
void print_string (char* str);

//============================================================
//===================== VECTORS ==============================
//============================================================

typedef struct {  
  int size;
  int capacity;
  void** array;
} Vector;

Vector* make_vector ();
void vector_add (Vector* v, void* val);
void* vector_pop (Vector* v);
void* vector_peek (Vector* v);
void vector_clear (Vector* v);
void vector_free (Vector* v);
void* vector_get (Vector* v, int i);
void vector_set (Vector* v, int i, void* x);
void vector_set_length (Vector* v, int len, void* x);

//============================================================
//===================== HASH MAP =============================
//============================================================

typedef struct {
  Vector** buckets;
  size_t size;
} HashMap;

typedef struct {
  char* key;
  void* val;
} HashNode;

HashMap* make_hashmap(size_t n);
void hashmap_add(HashMap* hm, char* key, void* data);
void* hashmap_get(HashMap* hm, char* key);
void hashmap_free(HashMap* hm);
void hashmap_print(HashMap* hm);

#endif
