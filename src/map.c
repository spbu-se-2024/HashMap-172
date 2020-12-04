#include <malloc.h>
#include <memory.h>
#include "map.h"

static const float DEFAULT_LOAD_FACTOR = 0.75f;

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
    int modification_count; // increments on each map update, makes outdated iterators fail fast  FIXME overflow checks
    size_t size;
    size_t threshold; // if size reaches threshold, map is resized
    float load_factor;
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

// doubles capacity
static void MAP_resize(MAP *map) {
    map->modification_count++;
    ENTRY **old_table = map->table;
    size_t old_capacity = map->capacity;
    map->capacity = old_capacity << 1u; // FIXME check for overflow
    map->threshold = map->capacity * map->load_factor;
    map->table = calloc(map->capacity, sizeof(ENTRY *));
    if (map->table == NULL) {
        map->status = (STATUS) {STATUS_OUT_OF_MEMORY, "resized table"};
        free(old_table);
        return;
    }
    for (size_t i = 0; i < old_capacity; i++) {
        ENTRY *lowHead = NULL, *lowTail = NULL, *highHead = NULL, *highTail = NULL;
        for (ENTRY *entry = old_table[i]; entry != NULL; entry = entry->next) {
            if (entry->hash & old_capacity) {
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
            map->table[i] = lowHead;
        }
        if (highTail != NULL) {
            highTail->next = NULL;
            map->table[i + old_capacity] = highHead;
        }
    }
    free(old_table);
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
    if (map->size++ > map->threshold) MAP_resize(map);
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

int MAP_fprint_stats(MAP *map, FILE *stream) {
    size_t chain_count = 0;
    for (size_t i = 0; i < map->capacity; i++)
        if (map->table[i] != NULL) chain_count++;
    // FIXME we may be supposed to print chain lengths distribution as well
    return fprintf(stream, "============\n"
                           "MAP stats:\n"
                           "status: ") + STATUS_fprint(&map->status, stream) +
           fprintf(stream, "capacity: %zu\n"
                           "threshold: %zu\n"
                           "size: %zu\n"
                           "chain count: %zu\n"
                           "average chain length: %f\n"
                           "modification count: %d\n"
                           "============\n",
                   map->capacity, map->threshold, map->size, chain_count,
                   map->size / (float) chain_count, map->modification_count);
}

void MAP_fprint_stats_or_fail(MAP *map, FILE *stream) {
    if (MAP_fprint_stats(map, stream) < 0)
        map->status = (STATUS) {PRINT_ERROR, "stats"};
}

bool MAP_log_and_free_on_error(MAP *map) {
    bool has_error = STATUS_log_on_error(&map->status, "map");
    if (has_error) MAP_free(map);
    return has_error;
}

struct entry_iterator_t {
    MAP *map;
    int expected_modification_count;
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
    while (map->capacity < capacity) map->capacity <<= 1u; // FIXME check for overflow
    map->table = calloc(map->capacity, sizeof(ENTRY *));
    if (map->table == NULL) {
        free(map);
        return NULL;
    }
    map->hash_function = hash_function;
    map->status = (STATUS) {STATUS_OK, NULL};
    map->modification_count = 0;
    map->size = 0;
    map->threshold = capacity * load_factor;
    map->load_factor = load_factor;
    return map;
}

MAP *new_MAP_with_default_load_factor(size_t capacity, hash_function_t hash_function) {
    return new_MAP(capacity, DEFAULT_LOAD_FACTOR, hash_function);
}
