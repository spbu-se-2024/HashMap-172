#include "hash_functions.h"

hash_t polynomial_hash(char *str) {
    hash_t hash = 0;
    while (*str != '\0')
        hash = 31 * hash + *str++;
    return hash;
}
