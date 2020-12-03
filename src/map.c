#include <malloc.h>
#include <memory.h>
#include "map.h"

bool are_equal_keys(map_key_t key1, map_key_t key2) {
    return strcmp(key1, key2) == 0;
}

map_key_t copy_key(map_key_t key) {
    size_t size = (strlen(key) + 1) * sizeof(char); // including '\0'
    map_key_t copy = malloc(size);
    if (copy == NULL) return NULL;
    memcpy(copy, key, size);
    return copy;
}

const_map_key_t ENTRY_get_key(ENTRY *self) {
    return self->funcs->get_key(self->data);
}

map_value_t ENTRY_get_value(ENTRY *self) {
    return self->funcs->get_value(self->data);
}

void ENTRY_set_value(ENTRY *self, map_value_t value) {
    self->funcs->set_value(self->data, value);
}

ENTRY *ENTRY_ITERATOR_next(ENTRY_ITERATOR *self) {
    return self->funcs->next(self->data);
}

void ENTRY_ITERATOR_free(ENTRY_ITERATOR *self) {
    self->funcs->free(self->data);
}

size_t MAP_size(MAP *self) {
    return self->funcs->size(self->data);
}

void MAP_clear(MAP *self) {
    self->funcs->clear(self->data);
}

void MAP_free(MAP *self) {
    self->funcs->free(self->data);
}

void MAP_put(MAP *self, map_key_t key, map_value_t value) {
    self->funcs->put(self->data, key, value);
}

map_value_t *MAP_get(MAP *self, map_key_t key) {
    return self->funcs->get(self->data, key);
}

ENTRY_ITERATOR *MAP_get_entry_iterator(MAP *self) {
    return self->funcs->get_entry_iterator(self->data);
}

int MAP_print_stats(MAP *self, FILE *stream) {
    return self->funcs->print_stats(self->data, stream);
}

MAP_STATUS MAP_get_status(MAP *self) {
    return self->funcs->get_status(self->data);
}

int MAP_printf_stats(MAP *self) {
    return MAP_print_stats(self, stdout);
}

bool MAP_is_ok(MAP *self) {
    return MAP_get_status(self).type == MAP_OK;
}

bool MAP_log_on_error(MAP *self) {
    if (MAP_is_ok(self)) return false;
    LOG_ERROR("Error occurred while using map:\n");
    MAP_print_status(self, stderr);
    MAP_print_stats(self, stderr); // FIXME do these stats make error message more informative?
    return true;
}

bool MAP_log_and_free_on_error(MAP *self) {
    bool has_errors = MAP_log_on_error(self);
    if (has_errors) free(self);
    return has_errors;
}

char *MAP_STATUS_TYPE_message_format(MAP_STATUS_TYPE self) {
    switch (self) {
        case MAP_OK: return "ok\n";
        case MAP_OUT_OF_MEMORY: return "Unable to allocate memory for %s\n";
        case MAP_CONCURRENT_MODIFICATION: return "Concurrent modification occurred while using %s\n";
        default: return "Unknown error\n";
    }
}

int MAP_STATUS_print(MAP_STATUS self, FILE *stream) {
    return fprintf(stream, MAP_STATUS_TYPE_message_format(self.type), self.info);
}

int MAP_print_status(MAP *self, FILE *stream) {
    return MAP_STATUS_print(MAP_get_status(self), stream);
}
