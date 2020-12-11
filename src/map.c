#include <malloc.h>
#include <memory.h>
#include "map.h"

/*
 * For all non-negative integers x < MAX_CAPACITY:
 *  - 2x must be within the range of size_t type
 *  - x must be within the range of HM172Hash type
 */
static const size_t MAX_CAPACITY = 1 + ((HASH_MAX < SIZE_MAX / 2) ? HASH_MAX : (SIZE_MAX / 2));

struct entry_t {
    HM172Key key;
    HM172Value value;
    HM172Hash hash;
    HM172Entry *next;
};

static bool are_equal_keys(HM172ConstKey key1, HM172ConstKey key2) {
    return strcmp(key1, key2) == 0;
}

static HM172Key copy_key(HM172ConstKey key) {
    size_t size = (strlen(key) + 1) * sizeof(char); // including '\0'
    HM172Key copy = malloc(size);
    if (copy == NULL) return NULL;
    memcpy(copy, key, size);
    return copy;
}

HM172ConstKey hm172_get_entry_key(HM172Entry *entry) {
    return entry->key;
}

HM172Value hm172_get_entry_value(HM172Entry *entry) {
    return entry->value;
}

void hm172_set_entry_value(HM172Entry *entry, HM172Value value) {
    entry->value = value;
}

static HM172Entry *new_entry(HM172Key key, HM172Value value, HM172Hash hash) {
    HM172Entry *entry = malloc(sizeof(HM172Entry));
    if (entry == NULL) return NULL;
    entry->key = key;
    entry->value = value;
    entry->hash = hash;
    entry->next = NULL;
    return entry;
}

struct map_t {
    HM172Entry **table;
    hash_function_t hash_function;
    HM172Status status;
    size_t capacity; // must be a power of 2, can't be 0
    unsigned modification_count; // increments on each map update, makes outdated iterators fail fast
    size_t size;
    float threshold; // negative if resizing disabled
    float load_factor; // negative if resizing disabled
};

static HM172Entry **get_entry_ptr_by_key_and_hash(HM172Map *map, HM172Key key, HM172Hash hash) {
    HM172Entry **entry_ptr = map->table + ((map->capacity - 1) & hash);
    while (*entry_ptr != NULL) {
        if ((*entry_ptr)->hash == hash && are_equal_keys(key, (*entry_ptr)->key)) return entry_ptr;
        entry_ptr = &((*entry_ptr)->next);
    }
    return entry_ptr;
}

static HM172Entry **get_entry_ptr_by_key(HM172Map *map, HM172Key key) {
    return get_entry_ptr_by_key_and_hash(map, key, map->hash_function(key));
}

static void update_threshold(HM172Map *map) {
    map->threshold = (map->capacity == MAX_CAPACITY || map->load_factor < 0) ? -1 : map->capacity * map->load_factor;
}

static void double_capacity(HM172Map *map) {
    map->modification_count++;
    size_t new_capacity = map->capacity << 1u;
    if(new_capacity > MAX_CAPACITY) {
        map->threshold = -1;
        return;
    }
    HM172Entry **new_table = calloc(new_capacity, sizeof(HM172Entry *));
    if (new_table == NULL) {
        map->status = (HM172Status) {STATUS_OUT_OF_MEMORY, "resized table"};
        return;
    }
    for (size_t i = 0; i < map->capacity; i++) {
        HM172Entry *lowHead = NULL, *lowTail = NULL, *highHead = NULL, *highTail = NULL;
        for (HM172Entry *entry = map->table[i]; entry != NULL; entry = entry->next) {
            if (entry->hash & map->capacity) {
                if (highTail == NULL) highHead = entry;
                else highTail->next = entry;
                highTail = entry;
            } else {
                if (lowTail == NULL) lowHead = entry;
                else lowTail->next = entry;
                lowTail = entry;
            }
        }
        if (lowTail != NULL) {
            lowTail->next = NULL;
            new_table[i] = lowHead;
        }
        if (highTail != NULL) {
            highTail->next = NULL;
            new_table[i + map->capacity] = highHead;
        }
    }
    free(map->table);
    map->table = new_table;
    map->capacity = new_capacity;
    update_threshold(map);
}

size_t hm172_size(HM172Map *map) {
    return map->size;
}

void hm172_put(HM172Map *map, HM172Key key, HM172Value value) {
    map->modification_count++;
    HM172Hash hash = map->hash_function(key);
    HM172Entry **entry_ptr = get_entry_ptr_by_key_and_hash(map, key, hash);
    if (*entry_ptr != NULL) {
        (*entry_ptr)->value = value;
        return;
    }
    HM172Key key_copy = copy_key(key);
    if (key_copy == NULL) {
        map->status = (HM172Status) {STATUS_OUT_OF_MEMORY, "key copy"};
        return;
    }
    HM172Entry *entry = new_entry(key_copy, value, hash);
    if (entry == NULL) {
        free(key_copy);
        map->status = (HM172Status) {STATUS_OUT_OF_MEMORY, "entry"};
        return;
    }
    *entry_ptr = entry;
    map->size++;
    if (map->threshold >= 0 && (float) map->size > map->threshold)
        double_capacity(map);
}

HM172Value *hm172_get(HM172Map *map, HM172Key key) {
    HM172Entry **entry_ptr = get_entry_ptr_by_key(map, key);
    return *entry_ptr == NULL ? NULL : &(*entry_ptr)->value;
}

void hm172_clear(HM172Map *map) {
    map->modification_count++;
    map->size = 0;
    for (size_t i = 0; i < map->capacity; i++) {
        HM172Entry *entry = map->table[i];
        if (entry != NULL) {
            map->table[i] = NULL;
            do {
                free(entry->key);
                HM172Entry *next = entry->next;
                free(entry);
                entry = next;
            } while (entry != NULL);
        }
    }
}

void hm172_free(HM172Map *map) {
    if (map != NULL) {
        hm172_clear(map);
        free(map->table);
        free(map);
    }
}

HM172Status hm172_get_status(HM172Map *map) {
    return map->status;
}

void hm172_fprint_stats(HM172Map *map, FILE *stream) {
    size_t chain_count = 0;
    for (size_t i = 0; i < map->capacity; i++)
        if (map->table[i] != NULL) chain_count++;
    if (fprintf(stream, "============\n"
                        "HM172Map stats:\n"
                        "capacity: %zu\n"
                        "threshold: %0.f\n"
                        "size: %zu\n"
                        "chain count: %zu\n"
                        "average chain length: %f\n"
                        "modification count: %u\n"
                        "============\n",
                map->capacity, map->threshold, map->size, chain_count,
                map->size / (float) chain_count, map->modification_count) < 0)
        map->status = (HM172Status) {PRINT_ERROR, "stats"};
}

bool hm172_is_ok(HM172Map *map) {
    return hm172_is_status_ok(&map->status);
}

bool hm172_log_on_error(HM172Map *map) {
    return hm172_log_status_on_error(&map->status, "map");
}

bool hm172_log_and_free_on_error(HM172Map *map) {
    bool has_error = hm172_log_on_error(map);
    if (has_error) hm172_free(map);
    return has_error;
}

struct entry_iterator_t {
    HM172Map *map;
    unsigned expected_modification_count;
    size_t next_index;
    HM172Entry *next_entry;
};

static void advance_next_index(HM172EntryIterator *entry) {
    while (entry->next_index < entry->map->capacity &&
           (entry->next_entry = entry->map->table[entry->next_index++]) == NULL) {}
}

HM172Entry *hm172_next_entry(HM172EntryIterator *iterator) {
    if (iterator->map->modification_count != iterator->expected_modification_count) {
        iterator->map->status = (HM172Status) {STATUS_CONCURRENT_MODIFICATION, "entry iterator"};
        return NULL;
    }
    if (iterator->next_entry == NULL) return NULL;
    HM172Entry *current = iterator->next_entry;
    if ((iterator->next_entry = iterator->next_entry->next) == NULL)
        advance_next_index(iterator);
    return current;
}

void hm172_free_entry_iterator(HM172EntryIterator *iterator) {
    free(iterator);
}

HM172EntryIterator *hm172_get_entry_iterator(HM172Map *map) {
    HM172EntryIterator *iterator = malloc(sizeof(HM172EntryIterator));
    if (iterator == NULL) {
        map->status = (HM172Status) {STATUS_OUT_OF_MEMORY, "entry iterator"};
        return NULL;
    }
    iterator->map = map;
    iterator->expected_modification_count = map->modification_count;
    iterator->next_index = 0;
    iterator->next_entry = NULL;
    advance_next_index(iterator);
    return iterator;
}

static size_t capacity_to_valid_capacity(size_t capacity) {
    if (capacity > MAX_CAPACITY) capacity = MAX_CAPACITY;
    size_t valid_capacity = 1;
    while (valid_capacity < capacity) valid_capacity <<= 1u;
    if (valid_capacity > MAX_CAPACITY) valid_capacity >>= 1u;
    return valid_capacity;
}

HM172Map *hm172_new_map(size_t capacity, float load_factor, hash_function_t hash_function) {
    HM172Map *map = malloc(sizeof(HM172Map));
    if (map == NULL) return NULL;
    map->capacity = capacity_to_valid_capacity(capacity);
    map->table = calloc(map->capacity, sizeof(HM172Entry *));
    if (map->table == NULL) {
        free(map);
        return NULL;
    }
    map->hash_function = hash_function;
    map->status = (HM172Status) {STATUS_OK, NULL};
    map->modification_count = 0;
    map->size = 0;
    map->load_factor = load_factor < 0 ? -1 : load_factor;
    update_threshold(map);
    return map;
}
