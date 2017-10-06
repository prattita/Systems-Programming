#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include <stdio.h>

/* Daniel J. Bernstein's "times 33" string hash function, from comp.lang.C;
   See https://groups.google.com/forum/#!topic/comp.lang.c/lSKWXiuNOAk */
unsigned long hash(char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

hashtable_t *make_hashtable(unsigned long size) {
  hashtable_t *ht = malloc(sizeof(hashtable_t));
  ht->size = size;
  ht->buckets = calloc(sizeof(bucket_t *), size);
  return ht;
}

void ht_put(hashtable_t *ht, char *key, void *val) {
  /* FIXME: the current implementation doesn't update existing entries */
  unsigned int idx = hash(key) % ht->size;
  bucket_t *b = malloc(sizeof(bucket_t));
  bucket_t *b2 = ht->buckets[idx]; /*used for the looping*/
  while (b2){
    if (strcmp(key,b2->key)==0){ /*the key already exists*/
      free(b2->val);
      free(key);
      b2->val = val;
      free(b);
      return;
    }	 
    b2 = b2->next;
  }
  free(b2);
  b->key = key;
  b->val = val;
  b->next = ht->buckets[idx];
  ht->buckets[idx] = b; 
}

void *ht_get(hashtable_t *ht, char *key) {
  unsigned int idx = hash(key) % ht->size;
  bucket_t *b = ht->buckets[idx];
  while (b) {
    if (strcmp(b->key, key) == 0) {
      return b->val;
    }
    b = b->next;
  }
  return NULL;
}

void ht_iter(hashtable_t *ht, int (*f)(char *, void *)) {
  bucket_t *b;
  unsigned long i;
  for (i=0; i<ht->size; i++) {
    b = ht->buckets[i];
    while (b) {
      if (!f(b->key, b->val)) {
        return ; // abort iteration
      }
      b = b->next;
    }
  }
}

void free_hashtable(hashtable_t *ht) {
  bucket_t *b;
  unsigned long i;
  for (i=0; i<ht->size;i++){
    b = ht->buckets[i];
    while(b){
      free(b->val);
      free(b->key);
      free(b);
      b = b->next;
    }
  }
  free(ht->buckets);
  free(ht);
  //free(b);
}

/* TODO */
void  ht_del(hashtable_t *ht, char *key) {
  unsigned int idx = hash(key) %ht->size;
  bucket_t *b;
  bucket_t *prior;
  unsigned long i;
  for (i=0; i<ht->size; i++){
    b = ht->buckets[i];
    prior = NULL;
    while(b){
      if (strcmp(key,b->key)==0){ //found the key
	if (!prior){ //the node is the first element in the Linked List
	  ht->buckets[idx] = b->next;
	}
	else{
	  prior->next = b->next;
	}
	free(b->val);
	free(b->key);
	free(b);
	return;
      }
      prior = b;
      b = b->next;
    }
  }
}

void  ht_rehash(hashtable_t *ht, unsigned long newsize) {
  hashtable_t *ht2 = make_hashtable(newsize);
  bucket_t *b;
  unsigned long i;
  for (i=0; i<ht->size;i++){
    b = ht->buckets[i];
    while (b) {
      ht_put(ht2, b->key, b->val);
      // free(b->key);
      // free(b->val);
      free(b);
      b = b->next;
    }
  }
  free(ht->buckets);
  *ht = *ht2;
  free(ht2);
}
