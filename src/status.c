#include <stdio.h>
#include "status.h"

char *hm172_status_type_to_message_format(HM172StatusType status_type) {
    switch (status_type) {
        case STATUS_OK: return "No errors\n";
        case STATUS_OUT_OF_MEMORY: return "Unable to allocate memory for %s\n";
        case STATUS_CONCURRENT_MODIFICATION: return "Concurrent modification occurred while using %s\n";
        case PRINT_ERROR: return "Unable to print %s\n";
        default: return "Unknown error\n";
    }
}

bool hm172_is_status_ok(HM172Status *status) {
    return status->type == STATUS_OK;
}

int hm172_fprint_status(HM172Status *status, FILE *stream) {
    return fprintf(stream, hm172_status_type_to_message_format(status->type), status->data);
}

bool hm172_log_status_on_error(HM172Status *status, const char *status_owner) {
    if (hm172_is_status_ok(status)) return false;
    if (status_owner != NULL) LOG_ERROR("Error occurred in %s:\n", status_owner);
    hm172_fprint_status(status, stderr);
    return true;
}
