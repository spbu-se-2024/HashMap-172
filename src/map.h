#ifndef HASHMAP_172_MAP_H
#define HASHMAP_172_MAP_H

#include <stdbool.h>
#include "hash.h"
#include "status.h"

typedef char *map_key_t;
typedef const char *const_map_key_t;
typedef int map_value_t;

typedef hash_t (*hash_function_t)(map_key_t key);

typedef struct map_t MAP;
typedef struct entry_t ENTRY;
typedef struct entry_iterator_t ENTRY_ITERATOR;

const_map_key_t ENTRY_get_key(ENTRY *entry);

map_value_t ENTRY_get_value(ENTRY *entry);

void ENTRY_set_value(ENTRY *entry, map_value_t value);

/*
 * Returns next entry pointer or NULL if there are no entries left
 * The returned entry can be used to both view and modify value of the corresponding mapping
 * Usage of returned entry after its key is removed from the map is UNDEFINED BEHAVIOUR
 * If the map has been directly modified after iterator creation, the map state type is set to CONCURRENT_MODIFICATION and NULL is returned
 * Direct map modifications are all map content modifications excluding the ones performed via entries or value pointers
 */
ENTRY *ENTRY_ITERATOR_next(ENTRY_ITERATOR *iterator);

void ENTRY_ITERATOR_free(ENTRY_ITERATOR *iterator);

/*
 * Returns new MAP instance
 * If load_factor is negative, resizing is disabled
 * If sufficient amount of memory can't be allocated, NULL is returned
 */
MAP *new_MAP(size_t capacity, float load_factor, hash_function_t hash_function);

size_t MAP_size(MAP *map);

/*
 * Removes all of the mappings from the map, makes all entries and value pointers invalid
 */
void MAP_clear(MAP *map);

void MAP_free(MAP *map);

/*
 * Makes the map associate the value with the data pointed by the key during function execution
 * If the key is NULL the BEHAVIOUR is UNDEFINED
 * If sufficient amount of memory can't be allocated, the map state type is set to OUT_OF_MEMORY
 */
void MAP_put(MAP *map, map_key_t key, map_value_t value);

/*
 * Returns pointer to the value to which the specified key is mapped, or NULL if this map contains no mapping for the key
 * The returned pointer can be used to both view and modify value to which the specified key is mapped
 * Dereferencing returned pointer after its key is removed from the map is UNDEFINED BEHAVIOUR
 * If the key is NULL the BEHAVIOUR is UNDEFINED
 */
map_value_t *MAP_get(MAP *map, map_key_t key);

/*
 * Returns the iterator that can be used to iterate over all map entries by passing it to ENTRY_ITERATOR_next function
 * Iteration order is unspecified
 * If sufficient amount of memory can't be allocated, the map state type is set to OUT_OF_MEMORY and NULL is returned
 */
ENTRY_ITERATOR *MAP_get_entry_iterator(MAP *map);

/*
 * Prints map statistics to the specified stream
 * If a writing error occurs, the map status type is set to PRINT_ERROR
 */
void MAP_fprint_stats(MAP *map, FILE *stream);

STATUS MAP_get_status(MAP *map);

bool MAP_is_ok(MAP *map);

/*
 * Returns true and logs error if a error has occurred during previous map usage
 */
bool MAP_log_on_error(MAP *map);

/*
 * Returns true, frees the map, and logs error if a error has occurred during previous map usage
 */
bool MAP_log_and_free_on_error(MAP *map);

#endif // HASHMAP_172_MAP_H
