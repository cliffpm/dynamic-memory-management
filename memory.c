/* memory.c
   Memory Homework, 24AU
   Copyright Cliff Pham 2024
*/

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "mem.h"  // outward facing functions
#include "mem_internal.h"  // private functions

// Default values for us to use later on
#define NODESIZE sizeof(freeNode)
#define MINCHUNK 16        // smallest allowable chunk of memory
#define BIGCHUNK 16000     // default of a very large chunk size

// Global variables for convenience
// these are static so outside code can't use them.
static freeNode* freeBlockList;  // points to list of available memory blocks
static uintptr_t totalMalloc;  // keeps track of memory allocated with malloc

/* The following functions need to be defined to meet the interface
   specified in mem.h.  These functions return or take the 'usable'
   memory addresses that a user would deal with.  They are called
   in the bench code.
*/

/* getmem returns the address of a usable block of memory that is
   at least size bytes large.  This code calls the helper function
   'get_block'
   Pre-condition: size is a positive integer
*/
void* getmem(uintptr_t size) {
  // check_heap();
  assert(size > 0);
  // make sure size is a multiple of MINCHUNK (16):
  if (size % MINCHUNK != 0) {
    size = size + MINCHUNK -(size % MINCHUNK);
  }
  uintptr_t block = get_block(size);
  if (block == 0) {
    return NULL;
  }
  return((void*)(block+NODESIZE));  // offset to get usable address
}

/* freemem uses the functions developed to add blocks to the 
   list of available free blocks to return a node to the list.
   The pointer 'p' is the address of usable memory, allocated using getmem
*/
void freemem(void* p) {
  if (!p) {
    return;
  }
  uintptr_t freeAddress = ((uintptr_t)p - NODESIZE);
  return_block(freeAddress);
  return;
}

uintptr_t get_block(uintptr_t size) {
  freeNode* itr = freeBlockList;
  while (itr  && itr->size < size) {
    itr = itr->next;
  }
  if (itr == NULL) {
    itr = new_block(size);
  }

  // split if block found exceed size requested
  if (itr->size > size) {
    split_node(itr, size);
  }
  // front case
  if (itr == freeBlockList) {
    freeBlockList = freeBlockList->next;
  } else {
  // traversa; case
    freeNode* removePtr = freeBlockList;
    while (removePtr->next != itr) {
      removePtr = removePtr->next;
    }
    removePtr->next = itr->next;  // skips over itr
    itr->next = NULL;  // remove full access of itr to list
  }
  return (uintptr_t)itr;
}


// creates a new memory block and adds into the list
freeNode* new_block(int size) {
  freeNode*block;
  if (BIGCHUNK > size) {
    block = (freeNode*)malloc(BIGCHUNK);
    block->size = BIGCHUNK - NODESIZE;
    totalMalloc += BIGCHUNK;
  } else {
    block = (freeNode*)malloc(size+ NODESIZE);
    block->size = (uintptr_t)size;
    totalMalloc += (uintptr_t)size + NODESIZE;
  }
  block->next = NULL;
  return_block((uintptr_t)block);
  return block;
}

// splits one node into two nodes according to the spec in place
void split_node(freeNode* n, uintptr_t size) {
  if (n == NULL) {
    return;
  }
  uintptr_t sizeRemaining = (n->size - size - NODESIZE);
  uintptr_t newAddress = (uintptr_t)n +size + NODESIZE;
  freeNode* newBlock = (freeNode*)newAddress;
  newBlock->size = sizeRemaining;
  newBlock->next = n->next;
  n->next = newBlock;
  n->size = size;
}

// inserts block in ascending order of memory address
void return_block(uintptr_t node) {
  freeNode* curr = (freeNode*)node;
  if (!freeBlockList && curr) {
    freeBlockList = curr;
    curr->next = NULL;
    return;
  }
  // Need to check for adjacency and merge after insertion
  if ((uintptr_t)curr < (uintptr_t)freeBlockList) {
    curr->next = freeBlockList;
    freeBlockList = curr;
    if (adjacent(curr) == 1) {
      curr->size += curr->next->size + NODESIZE;
      curr->next = curr->next->next;
    }
    return;
  }

  freeNode* itr = freeBlockList;
  freeNode* prev = freeBlockList;  // make NULL ?
  while (itr && (uintptr_t)curr > (uintptr_t)itr) {
    prev = itr;
    itr = itr->next;
  }

  curr->next = itr;
  prev->next = curr;

  if (adjacent(prev) == 1 && curr->next) {
    prev->size += curr->size + NODESIZE;
    prev->next = curr->next;
    curr = prev;
  }

  if (adjacent(curr) == 1 && curr->next->next) {
    curr->size += (curr->next->size) + NODESIZE;
    curr->next = curr->next->next;
  }
}

// returns 1 if a block and the proceeding block are adjacent in
// memory, and 0 if they're not
int adjacent(freeNode* node) {
  // COMPLETEME
  if ((uintptr_t)node + node->size + NODESIZE == (uintptr_t)node->next) {
    return 1;
  }
  return 0;
}

/* The following are utility functions that may prove useful to you.
   They should work as presented, so you can leave them as is.
*/
void check_heap() {
  if (!freeBlockList) return;
  freeNode* currentNode = freeBlockList;
  uintptr_t minsize = currentNode->size;

  while (currentNode != NULL) {
    if (currentNode->size < minsize) {
      minsize = currentNode->size;
    }
    if (currentNode->next != NULL) {
      assert((uintptr_t)currentNode <(uintptr_t)(currentNode->next));
      assert((uintptr_t)currentNode + currentNode->size + NODESIZE
              <(uintptr_t)(currentNode->next));
    }
    currentNode = currentNode->next;
  }
  // go through free list and check for all the things
  if (minsize == 0) print_heap( stdout);
  assert(minsize >= MINCHUNK);
}

void get_mem_stats(uintptr_t* total_size, uintptr_t* total_free,
                   uintptr_t* n_free_blocks) {
  *total_size = totalMalloc;
  *total_free = 0;
  *n_free_blocks = 0;

  freeNode* currentNode = freeBlockList;
  while (currentNode) {
    *n_free_blocks = *n_free_blocks + 1;
    *total_free = *total_free + (currentNode->size + NODESIZE);
    currentNode = currentNode->next;
  }
}

void print_heap(FILE *f) {
  printf("Printing the heap\n");
  freeNode* currentNode = freeBlockList;
  while (currentNode !=NULL) {
    fprintf(f, "%" PRIuPTR, (uintptr_t)currentNode);
    fprintf(f, ", size: %" PRIuPTR, currentNode->size);
    fprintf(f, ", next: %" PRIuPTR, (uintptr_t)currentNode->next);
    fprintf(f, "\n");
    currentNode = currentNode->next;
  }
}
