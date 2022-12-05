#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

void run_tests(char* testname, TEST tests[], int n) {
  printf("Found %d tests in %s:\n", n, testname);

  int i;
  for (i = 0; i < n; i++) {
    printf("\t>>> Starting to run `%s`\n", tests[i].name);

    time_t start, end;
    double diff;

    time(&start);

    tests[i].signature();

    time(&end);
    diff = difftime(end, start);
    printf("\t<<< Successfully finished running `%s` (%.2lf seconds)\n", tests[i].name,
           diff);
  }

  printf("\n");
}
