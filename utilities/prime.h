#ifndef PRIME_H
#define PRIME_H

/**
 * Return the next prime number n great that or equal to the argument
 * such that n -2 is also prime
 * @param x lower bound prime number
 * @return larger of the next twin prime
 */
int next_twin_prime(int x);

/**
 * Return the next prime number n great that or equal to the argument
 * @param x lower bound prime number
 * @return the next prime 
 */
int next_prime(int p);

#endif