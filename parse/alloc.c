#include <stdio.h>

#include "jbb-alloc.h"

#define BLOCKSIZE 1000000
struct block {
  struct block *next, *prev;
  char contents[BLOCKSIZE];
};

struct heap_struct {
  struct block *first, *current;
};

#define hs_bottom(hs) (void *)((hs)->current->contents)
#define hs_top(hs) (void *)(&((hs)->current->contents[BLOCKSIZE]))

static void add_block(struct heap_struct *hs) {
  if (hs->first != NULL && hs->current == NULL) {
    hs->current = hs->first;
  } else if (hs->first == NULL || hs->current->next == NULL) {
    struct block *new_block = (struct block *)malloc(sizeof(struct block));
    if (new_block == NULL) {
      fprintf(stderr,"fatal_error: out of memory\n");
      exit(1);
    }
    new_block->next = NULL;
    new_block->prev = hs->current;
    if (hs->current == NULL) {
      hs->first = new_block;
    } else {
      hs->current->next = new_block;
    }
    hs->current = new_block;
  } else {
    hs->current = hs->current->next;
  }
}

static void release_to_block(struct heap_struct *hs, void *mark) {
  if (mark == NULL) {
    hs->current = NULL;
    return;
  }
  while (hs->current != NULL && mark < hs_bottom(hs) || mark > hs_top(hs)) {
    hs->current = hs->current->prev;
  }
  if (hs == NULL) {
    fprintf(stderr,"illegal release of heap\n");
    exit(1);
  }
}
  
struct heap_struct the_heap = {0,0};
struct heap_struct the_stack = {0,0};

void *heap = 0;
void *heaptop = 0;
void *stack = 0;
void *stacktop = 0;

void *more_heap(int n) {
  /*fprintf(stderr,"(allocating another block for %d bytes of storage)\n",n);*/
  if (n >= BLOCKSIZE) {
    fprintf(stderr,"more_heap: fatal error: heap request too large: %d\n",n);
    exit(1);
  }
  add_block(&the_heap);
  heap = hs_bottom(&the_heap);
  heaptop = hs_top(&the_heap);
  return HALLOC(n);
}

void *more_stack(int n) {
  if (n >= BLOCKSIZE) {
    fprintf(stderr,"more_stack: fatal error: stack request too large: %d\n",n);
    exit(1);
  }
  add_block(&the_stack);
  stack = hs_bottom(&the_stack);
  stacktop = hs_top(&the_stack);
  return SALLOC(n);
}

void release(void *mark) {
  release_to_block(&the_stack,mark);
  if (mark == NULL) {
    stack = stacktop = NULL;
  } else {
    stack = mark;
    stacktop = hs_top(&the_stack);
  }
}
