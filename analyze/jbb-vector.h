#define VECTOR(type) struct { type *array; int length; }
#define VECTORALLOC(v,type,n) (v).array=(type *)HALLOC(n*sizeof(type)); (v).length=n
