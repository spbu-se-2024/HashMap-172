#ifndef HASH_MAP_MAP_H
#define HASH_MAP_MAP_H

#include <stdbool.h>
#include <stdio.h>

#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))
#define INTERFACE(name) typedef struct { void *data; const name ## _FUNCS *funcs; } name;

typedef char *map_key_t; // NULL keys are not permitted
typedef const char *const_map_key_t;
typedef int map_value_t;

bool are_equal_keys(map_key_t key1, map_key_t key2);

// NULL if out of memory
map_key_t copy_key(map_key_t key);

typedef struct {
    const_map_key_t (*get_key)(void *self);
    map_value_t (*get_value)(void *self);
    void (*set_value)(void *self, map_value_t value);
} ENTRY_FUNCS;

INTERFACE(ENTRY)

const_map_key_t ENTRY_get_key(ENTRY *self);
map_value_t ENTRY_get_value(ENTRY *self);
void ENTRY_set_value(ENTRY *self, map_value_t value);

typedef struct {
    ENTRY *
    (*next)(void *self); // NULL if there is no entries left, NULL and MAP_CONCURRENT_MODIFICATION if map has been modified
    void (*free)(void *self);
} ENTRY_ITERATOR_FUNCS;

INTERFACE(ENTRY_ITERATOR)

ENTRY *ENTRY_ITERATOR_next(ENTRY_ITERATOR *self);
void ENTRY_ITERATOR_free(ENTRY_ITERATOR *self);

typedef enum {
    MAP_OK,
    MAP_OUT_OF_MEMORY,
    MAP_CONCURRENT_MODIFICATION
} MAP_STATUS_TYPE;

typedef struct {
    MAP_STATUS_TYPE type;
    char *info;
} MAP_STATUS;

char *MAP_STATUS_TYPE_message_format(MAP_STATUS_TYPE self);
int MAP_STATUS_print(MAP_STATUS self, FILE *stream);

// TODO suggestion: add MAP#set_status(MAP_STATUS), MAP_PRINT_ERROR status, extension functions print(f)_stats_or_err()

typedef struct {
    size_t (*size)(void *self);
    void (*clear)(void *self);
    void (*free)(void *self);
    void (*put)(void *self, map_key_t key, map_value_t value);
    map_value_t *(*get)(void *self, map_key_t key);
    ENTRY_ITERATOR *(*get_entry_iterator)(void *self);
    int (*print_stats)(void *self, FILE *stream); // returns the total number of characters written
    MAP_STATUS (*get_status)(void *self);
} MAP_FUNCS;

INTERFACE(MAP)

size_t MAP_size(MAP *self);
void MAP_clear(MAP *self);
void MAP_free(MAP *self);
void MAP_put(MAP *self, map_key_t key, map_value_t value);
map_value_t *MAP_get(MAP *self, map_key_t key);
ENTRY_ITERATOR *MAP_get_entry_iterator(MAP *self);
int MAP_print_stats(MAP *self, FILE *stream);
MAP_STATUS MAP_get_status(MAP *self);

int MAP_printf_stats(MAP *self);
bool MAP_is_ok(MAP *self);
int MAP_print_status(MAP *self, FILE *stream);
bool MAP_log_on_error(MAP *self);
bool MAP_log_and_free_on_error(MAP *self);

#endif // HASH_MAP_MAP_H
