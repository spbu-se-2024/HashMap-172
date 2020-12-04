#include <stdio.h>
#include "status.h"

char *STATUS_TYPE_message_format(STATUS_TYPE status_type) {
    switch (status_type) {
        case STATUS_OK: return "ok\n";
        case STATUS_OUT_OF_MEMORY: return "Unable to allocate memory for %s\n";
        case STATUS_CONCURRENT_MODIFICATION: return "Concurrent modification occurred while using %s\n";
        case PRINT_ERROR: return "Unable to print %s\n";
        default: return "Unknown error\n";
    }
}

int STATUS_fprint(STATUS *status, FILE *stream) {
    return fprintf(stream, STATUS_TYPE_message_format(status->type), status->info);
}

bool STATUS_log_on_error(STATUS *status, const char *status_owner) {
    if (status->type == STATUS_OK) return false;
    if (status_owner != NULL) LOG_ERROR("Error occurred in %s:\n", status_owner);
    STATUS_fprint(status, stderr);
    return true;
}
