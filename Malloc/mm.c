/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define BLK_HDR_SIZE ALIGN(sizeof(blockHdr))
#define BLK_FTR_SIZE ALIGN(sizeof(blockFtr))

#define START_BLK ((blockHdr *)mem_heap_lo())/*Returns first block in the heap*/

typedef struct header blockHdr;
struct header{
  size_t size;
  blockHdr *next_p;
  blockHdr *prior_p;
};
typedef struct footer blockFtr;
struct footer{
  blockHdr *head_ref;
};
/*
typedef struct diff_block diff_block;
struct diff_block{
  size_t size;
  blockHdr *next_p;
  blockHdr *prior_p;
  size_t footer;
};
*/
void *find_fit(size_t size);
void printheap(); 
int maxFree; /*Variable used to keep track of the biggest number in the free list, to avoid unnecesary traversals through the free list*/
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  blockHdr *p = mem_sbrk(BLK_HDR_SIZE + BLK_FTR_SIZE);
  p->size = BLK_HDR_SIZE + BLK_FTR_SIZE;
  p->next_p = p;
  p->prior_p = p;
  blockFtr *fp = (blockFtr *)((char *)p + BLK_HDR_SIZE);
  fp->head_ref = p;
  maxFree = 0;
  return 0;
}

void print_heap(){ /*Iterate through the entire heap, navigate using the size of each block*/
  blockHdr *bp = mem_heap_lo();
  while (bp < (blockHdr *)mem_heap_hi()){
    printf("%s block at %p, size %d\n", (bp->size&1)?"allocated":"free", bp, (int)(bp->size &~1));
    bp = (blockHdr *)((char *)bp + (bp->size & ~1));/*& ~1 it so we only get the size*/
  }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
/*Dont forget to implement splitting*/
{
  size_t newsize = ALIGN(size + BLK_HDR_SIZE +BLK_FTR_SIZE);/*replacing(size+SIZE_T_SIZE)*/
  blockHdr *bp = find_fit(newsize);
  if (bp == NULL) { /*didnt find anything in the list*/
      bp = mem_sbrk(newsize);
      if ((long)bp == -1)
        return NULL;
      else 
        bp->size = newsize | 1;
        blockFtr *fp = (blockFtr *)((char*)bp + ((bp->size) & ~1) - BLK_FTR_SIZE);
        fp->head_ref = bp;
  }
  else { /*Found a fit for bp, so we remove bp from the freeList */
    /*Splitting*/
    if (((bp->size)&~1) - newsize >= 0x7 + BLK_HDR_SIZE + BLK_FTR_SIZE){ /*Minimum block size*/  
      blockHdr *rem = (blockHdr *)((char *)bp + newsize);
      rem->size = bp->size-newsize;
      rem->size &= ~1;
      bp->size = newsize;
      bp->size |= 1; /*Mark it as allocated*/
      ((blockFtr *)((char *)bp + ((bp->size)&~1) - BLK_FTR_SIZE))->head_ref = bp; /*Moving footer to its block*/
      blockFtr *remf = (blockFtr *)((char*)rem + ((rem->size) & ~1) - BLK_FTR_SIZE);/*Creating and placing rem's footer*/
      remf->head_ref = rem;
      /*bp->prior_p->next_p = bp->next_p;
      bp->next_p->prior_p = bp->prior_p;*/
      rem->next_p = bp->next_p;
      rem->prior_p = bp->prior_p;
      bp->prior_p->next_p = rem;
      bp->next_p->prior_p = rem;
    }
    else{/*We use the whole block*/
      bp->size |= 1; /*Mark it as allocated*/
      bp->prior_p->next_p = bp->next_p;
      bp->next_p->prior_p = bp->prior_p;
    }
  }


  return (char *)bp + BLK_HDR_SIZE; /*returns a pointer to the beggining of the payload*/
}

/*
 * For now, will look into other methods later
 */
void *find_fit(size_t size)
{
  blockHdr *p;
  /*Iterate until the end of the list && Until we find a block that it is big enough to hold the size of the request.*/
  if (size < maxFree){ /*If the size is bigger than the biggest element in the heap, we don't loop because we won't find the requeted freeblock.*/
    for (p = START_BLK->next_p; p != mem_heap_lo() && p->size < size; p = p->next_p);
    if (p != mem_heap_lo()){
      return p;
    }
    else
      return NULL;
  }
  else
    return NULL;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  blockHdr *bp = (blockHdr*)((char*)ptr - BLK_HDR_SIZE);
  blockHdr *head = mem_heap_lo();
  bp->size &= ~1;/*deallocate*/
  /*FOR COALESCING, CHECK PRIOR AND NEXT TO SEE IF THEY ARE FREE*/
  
  int next_alloc, prev_alloc;
  blockHdr *bp_next = ((blockFtr*)((char*)bp - BLK_FTR_SIZE))->head_ref;
  blockHdr *bp_prior = (blockHdr*)((char*)bp + bp->size);
  
  if (bp_next!=START_BLK) /*1 if next is allocated, 0 if next is free, same thought process applies to prev_alloc*/
    next_alloc = bp_next->size &1;
  else
    next_alloc = 1;
  if (bp_prior!=START_BLK)
    prev_alloc = bp_prior->size &1;
  else
    prev_alloc = 1;

  /*Next coalesce*/
  if (!next_alloc){
    bp_next->next_p->prior_p = bp_next->prior_p;
    bp_next->prior_p->next_p = bp_next->next_p;
    bp_next->size += bp->size;
    ((blockFtr *)((char *)bp + bp->size - BLK_FTR_SIZE))->head_ref = bp_next;
    bp = bp_next;
  }
  /*Prior coalesce*/
  if(!prev_alloc){
    if (((void *)((char *)bp + bp->size) <= mem_heap_hi())) {
      bp->size+= bp_prior->size;
      ((blockFtr *)((char *)bp + (bp->size) - BLK_FTR_SIZE))->head_ref = bp;
      bp_prior->next_p->prior_p = bp_prior->prior_p;
      bp_prior->prior_p->next_p = bp_prior->next_p;
    }
  }
  if (maxFree < bp->size) /*Updating maxFree*/
    maxFree = bp->size;
  if (bp->size>BLK_FTR_SIZE+BLK_HDR_SIZE){
  bp->next_p = head->next_p;
  bp->prior_p = head;
  head->next_p = bp;
  bp->next_p->prior_p =bp;
  }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
 size_t newsize = ALIGN(size + BLK_HDR_SIZE + BLK_FTR_SIZE);
  blockHdr *bp = (blockHdr *)((char *)ptr - BLK_HDR_SIZE);
  blockHdr *bp_next = (blockHdr *)((char *)bp + (bp->size &= ~1));
  if (((void *)((char *)bp + bp->size) >= mem_heap_hi())) {
    mem_sbrk(newsize - bp->size);
    bp->size = newsize;
    ((blockFtr *)((char *)bp + (bp->size&~1) - BLK_FTR_SIZE))->head_ref = bp;
    return ptr;
  }
  /*Check to see if next is free*/
  if (((bp_next->size)&1) == 0 && bp_next!=START_BLK) {
    if (bp_next->size + (bp->size&~1) > newsize) {
      if (((void *)((char *)bp + bp->size) <= mem_heap_hi())) {
        bp->size = (bp->size + bp_next->size) | 1;
        ((blockFtr *)((char *)bp + ((bp->size)&~1) - BLK_FTR_SIZE))->head_ref = bp;
        bp_next->prior_p->next_p = bp_next->next_p;
        bp_next->next_p->prior_p = bp_next->prior_p;
        return ptr;
      }
    }
  }
  if ((bp->size&~1) > newsize) {
    return ptr;
  }
  void *new_ptr = mm_malloc(size);
  memcpy(new_ptr,ptr,bp->size - BLK_HDR_SIZE - BLK_FTR_SIZE);
  mm_free(ptr);
  return new_ptr;
}
