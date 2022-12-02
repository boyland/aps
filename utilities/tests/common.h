#ifndef COMMON_H
#define COMMON_H

#include "stdbool.h"

#define TOTAL_COUNT ((int)1e2)

typedef void (*Test_Signature)();

typedef struct test {
  Test_Signature signature;
  char* name;
} TEST;

void run_tests(char* testname, TEST tests[], int n);

void assert_true(char* name, bool result);

#endif