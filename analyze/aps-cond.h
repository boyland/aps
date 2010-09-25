typedef struct condition {
  unsigned positive;
  unsigned negative;
} CONDITION;
#define ONE_BIT(v) (((v)&((v)-1)) == 0)

enum CONDcompare { CONDeq, CONDgt, CONDlt, CONDcomp, CONDnone };
extern enum CONDcompare cond_compare(CONDITION *, CONDITION *);
