#include "jbb.h"
#include "jbb-alist.h"
#include "jbb-alloc.h"

struct alist {
  ALIST next;
  void *key, *value;
};

ALIST acons(void *key, void *value, ALIST rest) {
  ALIST complete=(ALIST)HALLOC(sizeof(struct alist));
  complete->next = rest;
  complete->key = key;
  complete->value = value;
  return complete;
}

void *assoc(void *key, ALIST alist) {
  while (alist != NULL) {
    if (alist->key == key) return alist->value;
    alist=alist->next;
  }
  return NULL;
}

void *rassoc(void *value, ALIST alist) {
  while (alist != NULL) {
    if (alist->value == value) return alist->key;
    alist=alist->next;
  }
}

BOOL setassoc(void *key, void *value, ALIST alist) {
  while (alist != NULL) {
    if (alist->key == key) {
      alist->value = value;
      return OK;
    }
    alist=alist->next;
  }
  return NOT_OK;
}
