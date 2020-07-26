#include "prime.h"
#include <stdbool.h>
#include "stdlib.h"
#include "string.h"

#define INITIAL_TABLE_SIZE 9973
#define DOUBLE_SIZE(x) ((x << 1) + 1)

typedef struct
{
  bool *array;
  unsigned int size
} PRIMES;

/**
 * Holds the dynamically allocated array and its size
 */
static PRIMES primes = {NULL, 0};

/**
 * Create a boolean array "prime[0..n]" and initialize
 * all entries as true. A value in prime[i] will
 * finally be false if i is not a prime, else true.
 */
void sieve_of_eratosthenes(int n)
{
  primes.size = n;

  size_t bytes = n * sizeof(bool);
  if (primes.array == NULL)
  {
    primes.array = malloc(bytes);
  }
  else
  {
    primes.array = realloc(primes.array, bytes);
  }

  memset(primes.array, 1, bytes);

  primes.array[0] = false;
  primes.array[1] = false;

  int i, j;
  for (i = 2; i * i < n; i++)
  {
    // If primes[p] is not changed, then it is a prime
    if (primes.array[i] == true)
    {
      // Update all multiples of p
      for (j = i * i; j < n; j += i)
      {
        primes.array[j] = false;
      }
    }
  }
}

/**
 * Return the next prime number n great that or equal to the argument
 * such that n -2 is also prime
 */
int next_twin_prime(int x)
{
  // If array is not initialized
  if (primes.array == NULL)
  {
    sieve_of_eratosthenes(INITIAL_TABLE_SIZE);
  }

  // If array size is not enough then resize the array
  if (x >= primes.size)
  {
    int new_size = DOUBLE_SIZE(primes.size);

    // Resized array is also not enough
    if (new_size <= x)
    {
      new_size = DOUBLE_SIZE(x);
    }

    sieve_of_eratosthenes(new_size);
  }

  while (true)
  {
    int i;
    for (i = x; i < primes.size; i++)
    {
      if (primes.array[i] == true && primes.array[i - 2] == true)
      {
        return i;
      }
    }

    // Resize the prime array and try again
    sieve_of_eratosthenes(DOUBLE_SIZE(primes.size));
  }
}
