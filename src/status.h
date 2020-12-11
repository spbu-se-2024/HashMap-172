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
} HM172StatusType;

typedef struct {
    HM172StatusType type;
    char *data; // additional textual data required by status type
} HM172Status;

/*
 * Returns the message format corresponding to the status type
 * Returned format can contain one embedded string specifier that is to be replaced with additional textual data
 */
char *hm172_status_type_to_message_format(HM172StatusType status_type);

bool hm172_is_status_ok(HM172Status *status);

/*
 * Prints status to specified stream
 * On success, the total number of characters written is returned
 * If a writing error occurs, the error indicator (ferror) is set and a negative number is returned
 */
int hm172_fprint_status(HM172Status *status, FILE *stream);

/*
 * Returns true and logs error occurred in status_owner if status isn't ok∟
 * If status_owner is NULL it is ignored
 */
bool hm172_log_status_on_error(HM172Status *status, const char *status_owner);

#endif // HASHMAP_172_STATUS_H
