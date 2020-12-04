#ifndef HASH_MAP_STATUS_H
#define HASH_MAP_STATUS_H

#include <stdio.h>
#include <stdbool.h>

#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))

typedef enum {
    STATUS_OK,
    STATUS_OUT_OF_MEMORY,
    STATUS_CONCURRENT_MODIFICATION,
    PRINT_ERROR
} STATUS_TYPE;

typedef struct {
    STATUS_TYPE type;
    char *info;
} STATUS;

char *STATUS_TYPE_message_format(STATUS_TYPE status_type);
int STATUS_fprint(STATUS *status, FILE *stream);
bool STATUS_log_on_error(STATUS *status, const char *status_owner);

#endif // HASH_MAP_STATUS_H
