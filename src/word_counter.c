#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include "map.h"
#include "hash_map.h"
#include "hash_functions.h"

#define MAX_WORD_LENGTH 1024

static const int EXPECTED_ARG_COUNT = 1 + 2;
static const int INPUT_FILE_ARG_INDEX = 1;
static const int INIT_MAP_CAPACITY_ARG_INDEX = 2;

int str_to_size_t(char *str, size_t *result, const char *variable_name) {
    char *endptr;
    uintmax_t result_umax = strtoumax(str, &endptr, 0);
    if (*endptr != '\0') {
        LOG_ERROR("Invalid %s. \"%s\" is not a valid number\n", variable_name, str);
        return -1;
    }
    if (result_umax == UINTMAX_MAX || result_umax > SIZE_MAX) {
        LOG_ERROR("Invalid %s. %s is too large\n", variable_name, str);
        return -1;
    }
    *result = result_umax;
    return 0;
}

char *fread_word(FILE *stream, char *buffer, size_t buffer_length) {
    int ch;
    while (!isalpha(ch = fgetc(stream)))
        if (ch == EOF) return NULL;
    size_t i;
    for (i = 0; i < buffer_length - 1 && isalpha(ch); i++) {
        buffer[i] = (char) tolower(ch);
        ch = fgetc(stream);
    }
    buffer[i] = '\0';
    ungetc(ch, stream);
    return buffer;
}

// FIXME too long function
int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARG_COUNT) {
        LOG_ERROR("Invalid number of command line arguments. Expected %d arg(s), but found %d arg(s)\n"
                  "Usage: word_counter <input input_file> <initial map capacity>\n", EXPECTED_ARG_COUNT - 1, argc - 1);
        return -1;
    }
    size_t init_map_capacity;
    if (str_to_size_t(argv[INIT_MAP_CAPACITY_ARG_INDEX], &init_map_capacity, "initial map capacity") != 0)
        return -1;
    MAP *map = new_HASH_MAP_with_default_load_factor(init_map_capacity, simple_hash);
    if (map == NULL) {
        LOG_ERROR("Unable to allocate memory for hash map\n");
        return -1;
    }
    FILE *input_file = fopen(argv[INPUT_FILE_ARG_INDEX], "r");
    if (input_file == NULL) {
        LOG_ERROR("Unable to find input file \"%s\"\n", argv[INPUT_FILE_ARG_INDEX]);
        MAP_free(map);
        return -1;
    }
    char word[MAX_WORD_LENGTH];
    while (fread_word(input_file, word, MAX_WORD_LENGTH) != NULL) {
        int *count_ptr = MAP_get(map, word);
        MAP_put(map, word, count_ptr == NULL ? 1 : *count_ptr + 1);
        if (MAP_log_and_free_on_error(map)) {
            fclose(input_file);
            return -1;
        }
    }
    fclose(input_file);
    if (MAP_printf_stats(map) < 0) {
        LOG_ERROR("Unable to print map stats\n"); // FIXME this logic can be extracted to map.c
        MAP_free(map);
        return -1;
    }
    ENTRY_ITERATOR *iterator = MAP_get_entry_iterator(map);
    if (MAP_log_and_free_on_error(map)) return -1;
    ENTRY *most_common_word_entry = NULL, *cur_entry;
    while ((cur_entry = ENTRY_ITERATOR_next(iterator)) != NULL)
        if (most_common_word_entry == NULL || ENTRY_get_value(cur_entry) > ENTRY_get_value(most_common_word_entry))
            most_common_word_entry = cur_entry;
    ENTRY_ITERATOR_free(iterator);
    if (most_common_word_entry == NULL) {
        LOG_ERROR("No words were found in input file\n");
        MAP_free(map);
        return -1;
    }
    if (printf("Unique word count: %zu\n"
               "Most common word: %s (appears %d times)\n",
               MAP_size(map), ENTRY_get_key(most_common_word_entry), ENTRY_get_value(most_common_word_entry)) < 0) {
        LOG_ERROR("Unable to print results\n");
        MAP_free(map);
        return -1;
    }
    MAP_free(map);
}
