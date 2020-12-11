#ifndef HASHMAP_172_MAP_H
#define HASHMAP_172_MAP_H

#include <stdbool.h>
#include "hash.h"
#include "status.h"

typedef char *HM172Key;
typedef const char *HM172ConstKey;
typedef int HM172Value;

typedef HM172Hash (*hash_function_t)(HM172Key key);

typedef struct map_t HM172Map;
typedef struct entry_t HM172Entry;
typedef struct entry_iterator_t HM172EntryIterator;

HM172ConstKey hm172_get_entry_key(HM172Entry *entry);

HM172Value hm172_get_entry_value(HM172Entry *entry);

void hm172_set_entry_value(HM172Entry *entry, HM172Value value);

/*
 * Returns next entry pointer or NULL if there are no entries left
 * The returned entry can be used to both view and modify value of the corresponding mapping
 * Usage of returned entry after its key is removed from the map is UNDEFINED BEHAVIOUR
 * If the map has been directly modified after iterator creation, the map state type is set to CONCURRENT_MODIFICATION and NULL is returned
 * Direct map modifications are all map content modifications excluding the ones performed via entries or value pointers
 */
HM172Entry *hm172_next_entry(HM172EntryIterator *iterator);

void hm172_free_entry_iterator(HM172EntryIterator *iterator);

/*
 * Returns new HM172Map instance
 * If load_factor is negative, resizing is disabled
 * If sufficient amount of memory can't be allocated, NULL is returned
 */
HM172Map *hm172_new_map(size_t capacity, float load_factor, hash_function_t hash_function);

size_t hm172_size(HM172Map *map);

/*
 * Removes all of the mappings from the map, makes all entries and value pointers invalid
 */
void hm172_clear(HM172Map *map);

void hm172_free(HM172Map *map);

/*
 * Makes the map associate the value with the data pointed by the key during function execution
 * If the key is NULL the BEHAVIOUR is UNDEFINED
 * If sufficient amount of memory can't be allocated, the map state type is set to OUT_OF_MEMORY
 */
void hm172_put(HM172Map *map, HM172Key key, HM172Value value);

/*
 * Returns pointer to the value to which the specified key is mapped, or NULL if this map contains no mapping for the key
 * The returned pointer can be used to both view and modify value to which the specified key is mapped
 * Dereferencing returned pointer after its key is removed from the map is UNDEFINED BEHAVIOUR
 * If the key is NULL the BEHAVIOUR is UNDEFINED
 */
HM172Value *hm172_get(HM172Map *map, HM172Key key);

/*
 * Returns the iterator that can be used to iterate over all map entries by passing it to hm172_next_entry function
 * Iteration order is unspecified
 * If sufficient amount of memory can't be allocated, the map state type is set to OUT_OF_MEMORY and NULL is returned
 */
HM172EntryIterator *hm172_get_entry_iterator(HM172Map *map);

/*
 * Prints map statistics to the specified stream
 * If a writing error occurs, the map status type is set to PRINT_ERROR
 */
void hm172_fprint_stats(HM172Map *map, FILE *stream);

HM172Status hm172_get_status(HM172Map *map);

bool hm172_is_ok(HM172Map *map);

/*
 * Returns true and logs error if a error has occurred during previous map usage
 */
bool hm172_log_on_error(HM172Map *map);

/*
 * Returns true, frees the map, and logs error if a error has occurred during previous map usage
 */
bool hm172_log_and_free_on_error(HM172Map *map);

#endif // HASHMAP_172_MAP_H
