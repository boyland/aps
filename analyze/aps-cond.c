#include <stdio.h>
#include "jbb-alloc.h"
#include "aps-ag.h"

enum CONDcompare cond_compare(CONDITION *cond1, CONDITION *cond2) {
  if (cond1->positive == cond2->positive &&
      cond1->negative == cond2->negative) {
    return CONDeq;
  } else if ((cond1->positive|cond2->positive)==cond1->positive &&
	     (cond1->negative|cond2->negative)==cond1->negative) {
    return CONDlt;
  } else if ((cond2->positive|cond1->positive)==cond2->positive &&
	     (cond2->negative|cond1->negative)==cond2->negative) {
    return CONDgt;
  } else if ((cond1->positive|cond2->positive)==cond1->positive &&
	     (cond2->negative|cond1->negative)==cond2->negative) {
    unsigned pos = cond1->positive - cond2->positive;
    unsigned neg = cond2->negative - cond1->negative;
    if (pos == neg && ONE_BIT(pos)) return CONDcomp;
  } else if ((cond2->positive|cond1->positive)==cond2->positive &&
	     (cond1->negative|cond2->negative)==cond1->negative) {
    unsigned pos = cond2->positive - cond1->positive;
    unsigned neg = cond1->negative - cond2->negative;
    if (pos == neg && ONE_BIT(pos)) return CONDcomp;
  }
  return CONDnone;
}

void print_condition(CONDITION *cond, FILE *stream) {
  if (cond->positive != NULL) {
    fprintf(stream,"+0%o",cond->positive);
  } else if (cond->negative == NULL) {
    fprintf(stream,"_");
  }
  if (cond->negative != NULL) {
    fprintf(stream,"-0%o",cond->negative);
  }
}






