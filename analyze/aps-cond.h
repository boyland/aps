typedef struct condition {
  unsigned positive;
  unsigned negative;
} CONDITION;
#define ONE_BIT(v) (((v)&((v)-1)) == 0)

extern enum CONDcompare { CONDeq, CONDgt, CONDlt, CONDcomp, CONDnone }
cond_compare(CONDITION *, CONDITION *);
