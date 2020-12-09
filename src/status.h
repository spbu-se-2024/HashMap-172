#ifndef HASHMAP_172_STATUS_H
#define HASHMAP_172_STATUS_H

#include <stdio.h>
#include <stdbool.h>

#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))

typedef enum {
    STATUS_OK, // no data required
    STATUS_OUT_OF_MEMORY, // data must hold variable name
    STATUS_CONCURRENT_MODIFICATION, // data must hold the name of the entity that was unable to handle concurrent modification
    PRINT_ERROR // data must hold message name
} STATUS_TYPE;

typedef struct {
    STATUS_TYPE type;
    char *data; // additional textual data required by status type
} STATUS;

/*
 * Returns the message format corresponding to the status type
 * Returned format can contain one embedded string specifier that is to be replaced with additional textual data
 */
char *STATUS_TYPE_message_format(STATUS_TYPE status_type);

bool STATUS_is_ok(STATUS *status);

/*
 * Prints status to specified stream
 * On success, the total number of characters written is returned
 * If a writing error occurs, the error indicator (ferror) is set and a negative number is returned
 */
int STATUS_fprint(STATUS *status, FILE *stream);

/*
 * Returns true and logs error occurred in status_owner if status isn't ok∟
 * If status_owner is NULL it is ignored
 */
bool STATUS_log_on_error(STATUS *status, const char *status_owner);

#endif // HASHMAP_172_STATUS_H
