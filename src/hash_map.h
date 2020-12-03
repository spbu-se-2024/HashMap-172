#ifndef HASH_MAP_HASH_MAP_H
#define HASH_MAP_HASH_MAP_H

#include <stdint.h>
#include "map.h"

typedef uint32_t hash_t;
typedef hash_t (*hash_function_t)(map_key_t key);

// NULL if out of memory
MAP *new_HASH_MAP(size_t capacity, float load_factor, hash_function_t hash_function);

MAP *new_HASH_MAP_with_default_load_factor(size_t capacity, hash_function_t hash_function);

#endif // HASH_MAP_HASH_MAP_H
