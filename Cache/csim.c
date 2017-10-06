#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXLEN 80

typedef struct{
  int countLRU;
  int valid;
  unsigned int tag;
  int index;
  int offset;
}line;

/*functions*/
void help(); /*Prints helper function*/
void readfile(char *argFile); /*Function to read file*/ 
void create_cache();
void free_cache();
void eval(char *trace, unsigned long address, int size); /*Evaluates each line in the file*/
int evict(int index);


/*Global var*/
int hit, miss, eviction;
int s = 0;
int E = 0;
int b = 0;
int nextLRU = 0;
line** cache;

int main(int argc, char **argv)
{
  int v = 0;/*Verbose flag*/
  char *argFile = NULL;
  int c;
  //  printf("HereB4GEOPT\n");
  while ((c= getopt(argc, argv, "hvs:E:b:t:")) != EOF){
    switch(c){
    case 'h':
      help();
      break;
    case 'v':
      v = 1; /*-v was called it will be executed in the end*/
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      argFile = (char*)optarg;
      break;
    }
  }
  if (s==0 || E==0 || b==0 || !argFile){
    printf("./csim: Missing required command line argument\n");
    help();
  }
  //printf("After s,E,b have values, before cache creation\n");
  create_cache();
  //  printf("Cache created, b4 file has been read s= %d\n",s);
  readfile(argFile);
  // printf("File read\n");
  printSummary(hit, miss, eviction);
  free_cache();
  return 0;
}

void create_cache(){
  int S = 1<<s; /*2^s*/
  cache = malloc(S * sizeof(line*));
  for (int i = 0; i<S; i++){
    cache[i] = malloc(E * sizeof(line)); /*Alocating space for the numbers of lines per set i*/
    for (int j = 0; j<E; j++){
      cache[i][j].valid = 0;
    }
  }
  return;
}

void free_cache(){
  int S = 1<<s;
  for (int i = 0; i<S; i++){
    free(cache[i]);
  }
  free(cache);
  return;
}
void readfile(char *argFile){
  FILE *file;
  char trace[10];
  unsigned long address;
  // unsigned long tag;
  //  int offset, index;
  char comma;
  int size; 
  if ((file = fopen(argFile, "r")) == NULL) {
    printf("%s No such file",argFile);
  }
  while (fscanf(file, "%s %lx %c %d", trace, &address, &comma, &size) != EOF ){
    eval(trace, address, size);
    printf("%d\n",nextLRU);
  }
  fclose(file);
}

void eval(char *trace, unsigned long address, int size){
  int offset;
  int index;
  int i;
  int minLRU;
  unsigned long tag;
  unsigned long masks = (1<<s)-1;
  unsigned long maskb = (1<<b)-1;
  offset = address & maskb;
  index = (address>>(b)) & masks;
  tag = address>>(s+b);
  /*  printf("tag %ld index %d offset%d\n",tag, index, offset);*/
  /*Load - Store - for the purposes of thos lab they work the same,
   Load: Look for the tag and validness of the given id, if found hit, else miss. If we miss, go and retireve it from memory and store it in the cache, if cache is full evict the LRU.
  Store: Look for the tag and validness of the given id, if found hit, else miss. If we miss, go and store the data in the first open cache, if cahce full evict the LRU*/
  if (!strcmp(trace,"L")||(!strcmp(trace,"S"))){
    for (i = 0; i<E; i++){/*Check if it's on the cache*/
      if ((cache[index][i].valid==1) && (cache[index][i].tag==tag)){
	hit++;
	cache[index][i].countLRU = nextLRU;
	nextLRU++;
	return;
      }
    }
    /*If we are here, then it is not in the cache, and we need to 'get it from memory' and put it in the cache*/
    miss++;
    for (i = 0; i<E; i++){
      if (cache[index][i].valid==0){ /*Checks if there are any empty spots, before performing any eviction*/
	cache[index][i].valid = 1;
	cache[index][i].tag = tag;
	cache[index][i].countLRU = nextLRU;
	nextLRU++;
	return;
      }
    }
    /*If all of the cache slots are full, we need to evict something. I am using the function evict, which returns the index of the 'Least recently used' cache slot in the set.*/
    minLRU = evict(index);
    cache[index][minLRU].countLRU = nextLRU;
    cache[index][minLRU].tag = tag;
    nextLRU++;
    eviction++;
  }
  else if (!strcmp(trace,"M")){
    /*Look if the tag exists for the given id, if it does hit = Load it*/
    int loaded = 0;/*If it loaded, then we skip both miss and eviction*/
    int inCache = 0;/*If it is not loaded, we check whether we can insert the element without an eviction*/
    for (i = 0; i<E; i++){
      if ((cache[index][i].valid==1)&&(cache[index][i].tag==tag)){
	hit++;
	cache[index][i].countLRU = nextLRU;
	nextLRU++;
	loaded = 1;
	inCache = 1;
      }
    }
    /*If value was not there, try to put it into an empty slot in the cache*/
    if(!loaded){
	miss++;  
	for (i=0; i<E; i++){
	  if (cache[index][i].valid == 0){
	    cache[index][i].tag = tag;
	    cache[index][i].valid = 1;
	    cache[index][i].countLRU = nextLRU;
	    nextLRU++;
	    inCache = 1;
	  }
	}
    }
    /*No empty slots, so we evict the LRU item*/
    if (!inCache){
      minLRU = evict(index);
      cache[index][minLRU].countLRU = nextLRU;
      cache[index][minLRU].tag = tag;
      nextLRU++;
      eviction++;
    }
  
    /*Once the element has been evicted, we store it*/
    for (i=0;i<E;i++){
      if ((cache[index][i].valid==1)&&(cache[index][i].tag==tag)){
	hit++;
	cache[index][i].countLRU = nextLRU;
	nextLRU++;
	return;
      }
    }
  }
}

int evict(int index){
  int minLRU = 0;
  for (int i=0; i<E; i++){
    if (cache[index][minLRU].countLRU > cache[index][i].countLRU){
      minLRU = i;
    }
  }
  return minLRU;
}

void help(){
  printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
  printf("Options:\n");
  printf("  -h         Print this help message.\n");
  printf("  -v         Optional verbose flag\n");
  printf("  -s <num>   Number of set index bits.\n");
  printf("  -E <num>   Number of lines per set.\n");
  printf("  -b <num>   Number of block offset bits.\n");
  printf("  -t <file>  Trace file.\n\n");
  printf("Examples:\n");
  printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
  printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
  exit(0);
  return;
}
