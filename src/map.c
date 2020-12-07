#include <malloc.h>
#include <memory.h>
#include "map.h"

static const size_t MAX_CAPACITY = 1 + ((HASH_MAX < SIZE_MAX / 2) ? HASH_MAX : (SIZE_MAX / 2));

struct entry_t {
    map_key_t key;
    map_value_t value;
    hash_t hash;
    ENTRY *next;
};

static bool are_equal_keys(const_map_key_t key1, const_map_key_t key2) {
    return strcmp(key1, key2) == 0;
}

static map_key_t copy_key(const_map_key_t key) {
    size_t size = (strlen(key) + 1) * sizeof(char); // including '\0'
    map_key_t copy = malloc(size);
    if (copy == NULL) return NULL;
    memcpy(copy, key, size);
    return copy;
}

const_map_key_t ENTRY_get_key(ENTRY *entry) {
    return entry->key;
}

map_value_t ENTRY_get_value(ENTRY *entry) {
    return entry->value;
}

void ENTRY_set_value(ENTRY *entry, map_value_t value) {
    entry->value = value;
}

static ENTRY *new_ENTRY(map_key_t key, map_value_t value, hash_t hash) {
    ENTRY *entry = malloc(sizeof(ENTRY));
    if (entry == NULL) return NULL;
    entry->key = key;
    entry->value = value;
    entry->hash = hash;
    entry->next = NULL;
    return entry;
}

struct map_t {
    ENTRY **table;
    hash_function_t hash_function;
    STATUS status;
    size_t capacity; // must be a power of 2, can't be 0
    unsigned modification_count; // increments on each map update, makes outdated iterators fail fast
    size_t size;
    float threshold; // negative if resizing disabled
    float load_factor; // negative if resizing disabled
};

static ENTRY **MAP_entry_ptr_by_key_and_hash(MAP *map, map_key_t key, hash_t hash) {
    ENTRY **entry_ptr = map->table + ((map->capacity - 1) & hash);
    while (*entry_ptr != NULL) {
        if ((*entry_ptr)->hash == hash && are_equal_keys(key, (*entry_ptr)->key)) return entry_ptr;
        entry_ptr = &((*entry_ptr)->next);
    }
    return entry_ptr;
}

static ENTRY **MAP_entry_ptr_by_key(MAP *map, map_key_t key) {
    return MAP_entry_ptr_by_key_and_hash(map, key, map->hash_function(key));
}

static void MAP_update_threshold(MAP *map) {
    map->threshold = (map->capacity == MAX_CAPACITY || map->load_factor < 0) ? -1 : map->capacity * map->load_factor;
}

static void MAP_double_capacity(MAP *map) {
    map->modification_count++;
    size_t new_capacity = map->capacity << 1u;
    if(new_capacity > MAX_CAPACITY) {
        map->threshold = -1;
        return;
    }
    ENTRY **new_table = calloc(new_capacity, sizeof(ENTRY *));
    if (new_table == NULL) {
        map->status = (STATUS) {STATUS_OUT_OF_MEMORY, "resized table"};
        return;
    }
    for (size_t i = 0; i < map->capacity; i++) {
        ENTRY *lowHead = NULL, *lowTail = NULL, *highHead = NULL, *highTail = NULL;
        for (ENTRY *entry = map->table[i]; entry != NULL; entry = entry->next) {
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
    MAP_update_threshold(map);
}

size_t MAP_size(MAP *map) {
    return map->size;
}

void MAP_put(MAP *map, map_key_t key, map_value_t value) {
    map->modification_count++;
    hash_t hash = map->hash_function(key);
    ENTRY **entry_ptr = MAP_entry_ptr_by_key_and_hash(map, key, hash);
    if (*entry_ptr != NULL) {
        (*entry_ptr)->value = value;
        return;
    }
    map_key_t key_copy = copy_key(key);
    if (key_copy == NULL) {
        map->status = (STATUS) {STATUS_OUT_OF_MEMORY, "key copy"};
        return;
    }
    ENTRY *entry = new_ENTRY(key_copy, value, hash);
    if (entry == NULL) {
        free(key_copy);
        map->status = (STATUS) {STATUS_OUT_OF_MEMORY, "entry"};
        return;
    }
    *entry_ptr = entry;
    map->size++;
    if (map->threshold >= 0 && (float) map->size > map->threshold)
        MAP_double_capacity(map);
}

map_value_t *MAP_get(MAP *map, map_key_t key) {
    ENTRY **entry_ptr = MAP_entry_ptr_by_key(map, key);
    return *entry_ptr == NULL ? NULL : &(*entry_ptr)->value;
}

void MAP_clear(MAP *map) {
    map->modification_count++;
    map->size = 0;
    for (size_t i = 0; i < map->capacity; i++) {
        ENTRY *entry = map->table[i];
        if (entry != NULL) {
            map->table[i] = NULL;
            do {
                free(entry->key);
                ENTRY *next = entry->next;
                free(entry);
                entry = next;
            } while (entry != NULL);
        }
    }
}

void MAP_free(MAP *map) {
    if (map != NULL) {
        MAP_clear(map);
        free(map->table);
        free(map);
    }
}

STATUS MAP_get_status(MAP *map) {
    return map->status;
}

void MAP_fprint_stats(MAP *map, FILE *stream) {
    size_t chain_count = 0;
    for (size_t i = 0; i < map->capacity; i++)
        if (map->table[i] != NULL) chain_count++;
    // FIXME we may be supposed to print chain lengths distribution as well
    if (fprintf(stream, "============\n"
                        "MAP stats:\n"
                        "capacity: %zu\n"
                        "threshold: %0.f\n"
                        "size: %zu\n"
                        "chain count: %zu\n"
                        "average chain length: %f\n"
                        "modification count: %u\n"
                        "============\n",
                map->capacity, map->threshold, map->size, chain_count,
                map->size / (float) chain_count, map->modification_count) < 0)
        map->status = (STATUS) {PRINT_ERROR, "stats"};
}

bool MAP_log_and_free_on_error(MAP *map) {
    bool has_error = STATUS_log_on_error(&map->status, "map");
    if (has_error) MAP_free(map);
    return has_error;
}

struct entry_iterator_t {
    MAP *map;
    unsigned expected_modification_count;
    size_t next_index;
    ENTRY *next_entry;
};

static void ENTRY_advance_next_index(ENTRY_ITERATOR *entry) {
    while (entry->next_index < entry->map->capacity &&
           (entry->next_entry = entry->map->table[entry->next_index++]) == NULL) {}
}

ENTRY *ENTRY_ITERATOR_next(ENTRY_ITERATOR *entry) {
    if (entry->map->modification_count != entry->expected_modification_count) {
        entry->map->status = (STATUS) {STATUS_CONCURRENT_MODIFICATION, "entry iterator"};
        return NULL;
    }
    if (entry->next_entry == NULL) return NULL;
    ENTRY *current = entry->next_entry;
    if ((entry->next_entry = entry->next_entry->next) == NULL)
        ENTRY_advance_next_index(entry);
    return current;
}

void ENTRY_ITERATOR_free(ENTRY_ITERATOR *iterator) {
    free(iterator);
}

ENTRY_ITERATOR *MAP_get_entry_iterator(MAP *map) {
    ENTRY_ITERATOR *iterator = malloc(sizeof(ENTRY_ITERATOR));
    if (iterator == NULL) {
        map->status = (STATUS) {STATUS_OUT_OF_MEMORY, "entry iterator"};
        return NULL;
    }
    iterator->map = map;
    iterator->expected_modification_count = map->modification_count;
    iterator->next_index = 0;
    iterator->next_entry = NULL;
    ENTRY_advance_next_index(iterator);
    return iterator;
}

MAP *new_MAP(size_t capacity, float load_factor, hash_function_t hash_function) {
    MAP *map = malloc(sizeof(MAP));
    if (map == NULL) return NULL;
    map->capacity = 1;
    while (map->capacity < capacity && map->capacity << 1u <= MAX_CAPACITY)
        map->capacity <<= 1u;
    map->table = calloc(map->capacity, sizeof(ENTRY *));
    if (map->table == NULL) {
        free(map);
        return NULL;
    }
    map->hash_function = hash_function;
    map->status = (STATUS) {STATUS_OK, NULL};
    map->modification_count = 0;
    map->size = 0;
    map->load_factor = load_factor < 0 ? -1 : load_factor;
    MAP_update_threshold(map);
    return map;
}
