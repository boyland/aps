#ifndef JBB_VECTOR_H
#define JBB_VECTOR_H

#define VECTOR(type) struct { type *array; int length; }
#define VECTORALLOC(v,type,n) (v).array=(type *)HALLOC(n*sizeof(type)); (v).length=n

#endif
