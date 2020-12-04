#ifndef HASH_MAP_HASH_MAP_H
#define HASH_MAP_HASH_MAP_H

#include <stdbool.h>
#include "hash.h"
#include "status.h"

typedef char *map_key_t; // NULL keys are not permitted
typedef const char *const_map_key_t;
typedef int map_value_t;

typedef hash_t (*hash_function_t)(map_key_t key);

typedef struct map_t MAP;
typedef struct entry_t ENTRY;
typedef struct entry_iterator_t ENTRY_ITERATOR;

const_map_key_t ENTRY_get_key(ENTRY *entry);
map_value_t ENTRY_get_value(ENTRY *entry);
void ENTRY_set_value(ENTRY *entry, map_value_t value);

ENTRY *ENTRY_ITERATOR_next(ENTRY_ITERATOR *iterator);
void ENTRY_ITERATOR_free(ENTRY_ITERATOR *iterator);

// NULL if out of memory
MAP *new_MAP(size_t capacity, float load_factor, hash_function_t hash_function);
MAP *new_MAP_with_default_load_factor(size_t capacity, hash_function_t hash_function);

size_t MAP_size(MAP *map);
void MAP_clear(MAP *map);
void MAP_free(MAP *map);
void MAP_put(MAP *map, map_key_t key, map_value_t value);
map_value_t *MAP_get(MAP *map, map_key_t key);
ENTRY_ITERATOR *MAP_get_entry_iterator(MAP *map);
int MAP_fprint_stats(MAP *map, FILE *stream);
void MAP_fprint_stats_or_fail(MAP *map, FILE *stream);
STATUS MAP_get_status(MAP *map);

// true if there is error
bool MAP_log_and_free_on_error(MAP *map);

#endif // HASH_MAP_HASH_MAP_H
