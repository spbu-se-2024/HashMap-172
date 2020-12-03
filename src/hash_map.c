#include <malloc.h>
#include "hash_map.h"

static const float DEFAULT_LOAD_FACTOR = 0.75f;

typedef struct node_t {
    map_key_t key;
    map_value_t value;
    hash_t hash;
    struct node_t *next;
    ENTRY entry;
} NODE;

const_map_key_t NODE_get_key(NODE *self) {
    return self->key;
}

map_value_t NODE_get_value(NODE *self) {
    return self->value;
}

void NODE_set_value(NODE *self, map_value_t value) {
    self->value = value;
}

static const ENTRY_FUNCS node_funcs = {
        (const_map_key_t (*)(void *)) NODE_get_key,
        (map_value_t (*)(void *)) NODE_get_value,
        (void (*)(void *, map_value_t)) NODE_set_value
};

// NULL if out of memory
NODE *new_NODE(map_key_t key, map_value_t value, hash_t hash) {
    NODE *self = malloc(sizeof(NODE));
    if (self == NULL) return NULL;
    self->key = key;
    self->value = value;
    self->hash = hash;
    self->next = NULL;
    self->entry = (ENTRY) {self, &node_funcs};
    return self;
}

typedef struct {
    NODE **table;
    hash_function_t hash_function;
    MAP_STATUS status;
    size_t capacity; // must be a power of 2, can't be 0
    int modification_count; // increments on each map update, makes outdated iterators fail fast
    size_t size;
    size_t threshold; // if size reaches threshold, map is resized
    float load_factor;
    MAP map;
} HASH_MAP;

NODE **HASH_MAP_node_ptr_by_key_and_hash(HASH_MAP *self, map_key_t key, hash_t hash) {
    NODE **node_ptr = self->table + ((self->capacity - 1) & hash);
    while (*node_ptr != NULL) {
        if ((*node_ptr)->hash == hash && are_equal_keys(key, (*node_ptr)->key)) return node_ptr;
        node_ptr = &((*node_ptr)->next);
    }
    return node_ptr;
}

NODE **HASH_MAP_node_ptr_by_key(HASH_MAP *self, map_key_t key) {
    return HASH_MAP_node_ptr_by_key_and_hash(self, key, self->hash_function(key));
}

// doubles capacity
void HASH_MAP_resize(HASH_MAP *self) /* throws MAP_OUT_OF_MEMORY */ {
    self->modification_count++;
    NODE **old_table = self->table;
    size_t old_capacity = self->capacity;
    self->capacity = old_capacity << 1u; // FIXME check for overflow
    self->threshold = self->capacity * self->load_factor;
    self->table = calloc(self->capacity, sizeof(NODE *));
    if (self->table == NULL) {
        self->status = (MAP_STATUS) {MAP_OUT_OF_MEMORY, "resized table"};
        return;
    }
    for (size_t i = 0; i < old_capacity; i++) {
        NODE *lowHead = NULL, *lowTail = NULL, *highHead = NULL, *highTail = NULL;
        for (NODE *node = old_table[i]; node != NULL; node = node->next) {
            if (node->hash & old_capacity) {
                if (highTail == NULL) highHead = node;
                else highTail->next = node;
                highTail = node;
            } else {
                if (lowTail == NULL) lowHead = node;
                else lowTail->next = node;
                lowTail = node;
            }
        }
        if (lowTail != NULL) {
            lowTail->next = NULL;
            self->table[i] = lowHead;
        }
        if (highTail != NULL) {
            highTail->next = NULL;
            self->table[i + old_capacity] = highHead;
        }
    }
    free(old_table);
}

size_t HASH_MAP_size(HASH_MAP *self) {
    return self->size;
}

void HASH_MAP_put(HASH_MAP *self, map_key_t key, map_value_t value) /* throws MAP_OUT_OF_MEMORY */ {
    self->modification_count++;
    if (!MAP_is_ok(&self->map)) return;
    hash_t hash = self->hash_function(key);
    NODE **node_ptr = HASH_MAP_node_ptr_by_key_and_hash(self, key, hash);
    if (*node_ptr != NULL) {
        (*node_ptr)->value = value;
        return;
    }
    map_key_t key_copy = copy_key(key);
    if (key_copy == NULL) {
        self->status = (MAP_STATUS) {MAP_OUT_OF_MEMORY, "key copy"};
        return;
    }
    NODE *node = new_NODE(key_copy, value, hash);
    if (node == NULL) {
        free(key_copy);
        self->status = (MAP_STATUS) {MAP_OUT_OF_MEMORY, "node"};
        return;
    }
    *node_ptr = node;
    if (self->size++ > self->threshold) HASH_MAP_resize(self);
}

map_value_t *HASH_MAP_get(HASH_MAP *self, map_key_t key) {
    NODE **node_ptr = HASH_MAP_node_ptr_by_key(self, key);
    return *node_ptr == NULL ? NULL : &(*node_ptr)->value;
}

void HASH_MAP_clear(HASH_MAP *self) {
    self->modification_count++;
    self->size = 0;
    for (size_t i = 0; i < self->capacity; i++) {
        NODE *node = self->table[i];
        if (node != NULL) {
            self->table[i] = NULL;
            do {
                free(node->key);
                NODE *next = node->next;
                free(node);
                node = next;
            } while (node != NULL);
        }
    }
}

void HASH_MAP_free(HASH_MAP *self) {
    if (self != NULL) {
        HASH_MAP_clear(self);
        free(self->table);
        free(self);
    }
}

MAP_STATUS HASH_MAP_get_status(HASH_MAP *self) {
    return self->status;
}

int HASH_MAP_print_stats(HASH_MAP *self, FILE *stream) {
    size_t chain_count = 0;
    for (size_t i = 0; i < self->capacity; i++)
        if (self->table[i] != NULL) chain_count++;
    // FIXME we may be supposed to print chain lengths distribution as well
    return fprintf(stream, "============\n"
                           "HASH_MAP stats:\n"
                           "status: ") + MAP_STATUS_print(self->status, stream) +
           fprintf(stream, "capacity: %zu\n"
                           "threshold: %zu\n"
                           "size: %zu\n"
                           "chain count: %zu\n"
                           "average chain length: %f\n"
                           "modification count: %d\n"
                           "============\n",
                   self->capacity, self->threshold, self->size, chain_count,
                   self->size / (float) chain_count, self->modification_count);
}

typedef struct {
    HASH_MAP *map;
    int expected_modification_count;
    size_t next_index;
    NODE *next_node;
    ENTRY_ITERATOR entry_iterator;
} NODE_ITERATOR;

void NODE_ITERATOR_free(NODE_ITERATOR *self) {
    free(self);
}

void NODE_advance_next_index(NODE_ITERATOR *self) {
    while (self->next_index < self->map->capacity &&
           (self->next_node = self->map->table[self->next_index++]) == NULL) {}
}

NODE *NODE_ITERATOR_next_node(NODE_ITERATOR *self) /* throws MAP_CONCURRENT_MODIFICATION */ {
    if (self->map->modification_count != self->expected_modification_count) {
        self->map->status = (MAP_STATUS) {MAP_CONCURRENT_MODIFICATION, "node iterator"};
        return NULL;
    }
    if (self->next_node == NULL) return NULL;
    NODE *current = self->next_node;
    if ((self->next_node = self->next_node->next) == NULL)
        NODE_advance_next_index(self);
    return current;
}

ENTRY *NODE_ITERATOR_next_entry(NODE_ITERATOR *self) /* throws MAP_CONCURRENT_MODIFICATION */ {
    NODE *node = NODE_ITERATOR_next_node(self);
    return node == NULL ? NULL : &node->entry;
}

static ENTRY_ITERATOR_FUNCS node_iterator_funcs = {
        (ENTRY *(*)(void *)) NODE_ITERATOR_next_entry,
        (void (*)(void *)) NODE_ITERATOR_free
};

// NULL and MAP_OUT_OF_MEMORY if out of memory
NODE_ITERATOR *new_NODE_ITERATOR(HASH_MAP *map) {
    NODE_ITERATOR *self = malloc(sizeof(NODE_ITERATOR));
    if (self == NULL) {
        map->status = (MAP_STATUS) {MAP_OUT_OF_MEMORY, "node iterator"};
        return NULL;
    }
    self->map = map;
    self->expected_modification_count = map->modification_count;
    self->next_index = 0;
    self->next_node = NULL;
    NODE_advance_next_index(self);
    self->entry_iterator = (ENTRY_ITERATOR) {self, &node_iterator_funcs};
    return self;
}

ENTRY_ITERATOR *HASH_MAP_get_entry_iterator(HASH_MAP *self) /* throws MAP_OUT_OF_MEMORY */ {
    NODE_ITERATOR *node_iterator = new_NODE_ITERATOR(self);
    return node_iterator == NULL ? NULL : &node_iterator->entry_iterator;
}

static MAP_FUNCS hash_map_funcs = {
        (size_t (*)(void *)) HASH_MAP_size,
        (void (*)(void *)) HASH_MAP_clear,
        (void (*)(void *)) HASH_MAP_free,
        (void (*)(void *, map_key_t, map_value_t)) HASH_MAP_put,
        (map_value_t *(*)(void *, map_key_t)) HASH_MAP_get,
        (ENTRY_ITERATOR *(*)(void *)) HASH_MAP_get_entry_iterator,
        (int (*)(void *, FILE *)) HASH_MAP_print_stats,
        (MAP_STATUS (*)(void *)) HASH_MAP_get_status
};

MAP *new_HASH_MAP(size_t capacity, float load_factor, hash_function_t hash_function) {
    HASH_MAP *self = malloc(sizeof(HASH_MAP));
    if (self == NULL) return NULL;
    self->capacity = 1;
    while (self->capacity < capacity) self->capacity <<= 1u; // FIXME check for overflow
    self->table = calloc(self->capacity, sizeof(NODE *));
    if (self->table == NULL) {
        free(self);
        return NULL;
    }
    self->hash_function = hash_function;
    self->status = (MAP_STATUS) {MAP_OK, NULL};
    self->modification_count = 0;
    self->size = 0;
    self->threshold = capacity * load_factor;
    self->load_factor = load_factor;
    self->map = (MAP) {self, &hash_map_funcs};
    return &self->map;
}

MAP *new_HASH_MAP_with_default_load_factor(size_t capacity, hash_function_t hash_function) {
    return new_HASH_MAP(capacity, DEFAULT_LOAD_FACTOR, hash_function);
}
