#ifndef HASHMAP_172_HASH_FUNCTIONS_H
#define HASHMAP_172_HASH_FUNCTIONS_H

#include "hash.h"

/*
 * Returns str[0]*p^(n-1) + str[1]*p^(n-2) + ... + str[n-1], where n is a length of the str, ^ is exponentiation
 */
hash_t polynomial_hash(char *str);

#endif // HASHMAP_172_HASH_FUNCTIONS_H
