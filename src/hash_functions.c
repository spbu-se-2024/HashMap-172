#include "hash_functions.h"

hash_t simple_hash(char *str) {
    int hash = 0;
    while (*str != '\0')
        hash = 31 * hash + *str++;
    return hash;
}
