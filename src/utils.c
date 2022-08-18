#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"

//============================================================
//===================== CONVENIENCE ==========================
//============================================================

int max (int a, int b) {
  return a > b? a : b;
}

int min (int a, int b) {
  return a < b? a : b;
}

void print_string (char* str) {
  printf("\"");
  while(1){
    char c = str[0];
    str++;
    switch(c){
    case '\n':
      printf("\\n");
      break;
    case '\\':
      printf("\\\\");
      break;
    case '"':
      printf("\\\"");
      break;
    case 0:
      printf("\"");
      return;
    default:
      printf("%c", c);
      break;
    }
  }
}

//============================================================
//===================== VECTORS ==============================
//============================================================

Vector* make_vector () {
  Vector* v = (Vector*)malloc(sizeof(Vector));
  v->size = 0;
  v->capacity = 8;
  v->array = malloc(sizeof(void*) * v->capacity);
  return v;
}

void vector_ensure_capacity (Vector* v, int c) {
  if(v->capacity < c){
    int c2 = max(v->capacity * 2, c);
    void** a2 = malloc(sizeof(void*) * c2);
    memcpy(a2, v->array, sizeof(void*) * v->size);
    free(v->array);
    v->capacity = c2;
    v->array = a2;
  }
}

void vector_set_length (Vector* v, int len, void* x) {
  if(len < 0){
    printf("Negative length given to vector.\n");
    exit(-1);
  }
  if(len <= v->size){
    v->size = len;
  }else{
    while(v->size < len)
      vector_add(v, x);
  }
}

void vector_add (Vector* v, void* val) {
  vector_ensure_capacity(v, v->size + 1);
  v->array[v->size] = val;
  v->size++;
}

void* vector_pop (Vector* v) {
  if(v->size == 0){
    printf("Pop from empty vector.\n");
    exit(-1);
  }  
  v->size--;
  return v->array[v->size];
}

void* vector_peek (Vector* v) {
  if(v->size == 0){
    printf("Peek from empty vector.\n");
    exit(-1);
  }  
  return v->array[v->size - 1];
}

void vector_clear (Vector* v){
  v->size = 0;
}

void vector_free (Vector* v){
  free(v->array);
  free(v);
}

void* vector_get (Vector* v, int i){
  if(i < 0 || i >= v->size){
    printf("Index %d out of bounds.\n", i);
    exit(-1);
  }
  return v->array[i];    
}

void vector_set (Vector* v, int i, void* x){
  if(i < 0 || i > v->size){
    printf("Index %d out of bounds.\n", i);
    exit(-1);
  }else if(i == v->size){
    vector_add(v, x);
  }else{
    v->array[i] = x;
  }
}



//============================================================
//===================== HASH MAP =============================
//============================================================

 
unsigned int hash(void *string)
{
	/* This is the djb2 string hash function */

	unsigned int result = 5381;
	unsigned char *p;

	p = (unsigned char *) string;

	while (*p != '\0') {
		result = (result << 5) + result + *p;
		++p;
	}
	return result;
}

HashMap* make_hashmap(size_t n) {
  HashMap* hm = malloc(sizeof(HashMap));
  Vector** buckets = malloc(sizeof(Vector*)*n);
  for (size_t i=0;i<n;i++) {
    buckets[i] = make_vector();
  }
  hm->buckets = buckets;
  hm->size = n;
  return hm;
}


void hashmap_add(HashMap* hm, char* key, void* data){
  Vector* bucket = hm->buckets[hash(key) % hm->size];
  for(int i=0;i<bucket->size;i++) {
    HashNode* curr = (HashNode*)vector_get(bucket, i);
    if (!strcmp(key, curr->key)) {
      curr->val = data; 
      return; 
    }
  }
  HashNode* node = (malloc(sizeof(HashNode)));
  node->key = key;
  node->val = data;
  vector_add(bucket, node);
}

void* hashmap_get(HashMap* hm, char* key) {
  Vector* bucket = hm->buckets[hash(key) % hm->size];
  for(int i=0;i<bucket->size;i++) {
    HashNode* curr = (HashNode*)vector_get(bucket, i);
    if (!strcmp(key, curr->key)) {
      return curr->val;
    }
  }
  return NULL;
}

void hashmap_free(HashMap* hm) {
  for (size_t i=0;i<hm->size;i++) {
    for (int j=0;j<hm->buckets[i]->size;j++) {
      free(vector_get(hm->buckets[i], j));
    }
    vector_free(hm->buckets[i]);
  }
  free(hm->buckets);
  free(hm);

}
void hashmap_print(HashMap* hm) {
  for (int i=0;i<hm->size;i++) {
    printf("%i: ", i);
    for (int j=0;j<hm->buckets[i]->size;j++) {
      printf("%s, ", ((HashNode*)(vector_get(hm->buckets[i], j)))->key);
    }
    printf("\n");
  }
}
