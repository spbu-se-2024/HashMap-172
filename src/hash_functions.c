#include "hash_functions.h"

HM172Hash hm172_polynomial_hash(char *str) {
    HM172Hash hash = 0;
    while (*str != '\0')
        hash = 31 * hash + *str++;
    return hash;
}
