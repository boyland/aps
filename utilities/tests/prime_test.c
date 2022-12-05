#include "../prime.h"
#include "common.h"

static void test_few_twin_primes() {
  assert_true("next twin prime of 1 should be", next_twin_prime(1) == 5);
  assert_true("next twin prime of 2 should be", next_twin_prime(2) == 5);
  assert_true("next twin prime of 3 should be", next_twin_prime(3) == 5);
  assert_true("next twin prime of 4 should be", next_twin_prime(4) == 5);
  assert_true("next twin prime of 5 should be", next_twin_prime(5) == 5);
  assert_true("next twin prime of 6 should be", next_twin_prime(6) == 7);
  assert_true("next twin prime of 7 should be", next_twin_prime(7) == 7);
  assert_true("next twin prime of 8 should be", next_twin_prime(8) == 13);
  assert_true("next twin prime of 9 should be", next_twin_prime(9) == 13);
  assert_true("next twin prime of 10 should be", next_twin_prime(10) == 13);
  assert_true("next twin prime of 11 should be", next_twin_prime(11) == 13);
  assert_true("next twin prime of 12 should be", next_twin_prime(12) == 13);
  assert_true("next twin prime of 13 should be", next_twin_prime(13) == 13);
}

void test_prime() {
  TEST tests[] = {{test_few_twin_primes, "test few twin primes"}};

  run_tests("prime", tests, 1);
}