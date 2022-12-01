#ifndef JBB_ALLOC_H
#define JBB_ALLOC_H

extern void *heap, *heaptop, *more_heap(int);
extern void *stack, *stacktop, *more_stack(int);
extern void release(void *); /* release things from stack */

static int ntemp;
static void *temp;
#define ALIGN(size) (((size)+7)&~7)
#define ALLOC(n,p,top,more) \
  (((p=(char *)(temp=p)+ALIGN(ntemp=n))>top)?more(ntemp):temp)

#define HALLOC(n) ALLOC(n,heap,heaptop,more_heap)
#define SALLOC(n) ALLOC(n,stack,stacktop,more_stack)

#endif

    
