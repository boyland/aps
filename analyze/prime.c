#include "prime.h"
#include "stdlib.h"
#include <stdbool.h>
#include "string.h"

#define INITIAL_TABLE_SIZE 9973

bool *primes = 0;

/**
 * Create a boolean array "prime[0..n]" and initialize
 * all entries it as true. A value in prime[i] will
 * finally be false if i is Not a prime, else true.
 */
void initialize_sieve_of_eratosthenes(int n)
{
  primes = malloc(n * sizeof(int));
  memset(primes, true, sizeof(primes));

  for (int p = 2; p < n / p; p++)
  {
    // If primes[p] is not changed, then it is a prime
    if (primes[p] == true)
    {
      // Update all multiples of p
      for (int i = p * 2; i < n; i += p) primes[i] = false;
    }
  }
}

/**
 * Return the next prime greater than parameter such that -2 is also a prime
 */
int next_twin_prime(int x)
{
  if (primes == 0)
  {
    initialize_sieve_of_eratosthenes(INITIAL_TABLE_SIZE);
  }

  if (x >= sizeof(primes))
  {
    initialize_sieve_of_eratosthenes((sizeof(primes) << 1) + 1);
  }

  int i;
  for (i = 0; i < sizeof(primes); i++)
  {
    if (primes[i - 2]) return i;
  }
}
