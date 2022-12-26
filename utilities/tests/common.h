#ifndef COMMON_H
#define COMMON_H

#include "stdbool.h"
#include "assert.h"

#define TOTAL_COUNT ((int)1<<10)

#define _ASSERT_EXPR(message, test) assert(((message), test))

typedef void (*Test_Signature)();

typedef struct test {
  Test_Signature signature;
  char* name;
} TEST;

void run_tests(char* testname, TEST tests[], int n);

#endif
