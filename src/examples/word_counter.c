#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include "../status.h"
#include "../map.h"
#include "../hash_functions.h"

#define MAX_WORD_LENGTH 1024

static const int MIN_ARG_COUNT = 1 + 3;
static const int INPUT_FILE_ARG_INDEX = 1;
static const int LOAD_FACTOR_ARG_INDEX = 2;
static const int FIRST_CAPACITY_ARG_INDEX = 3;

int str_to_size_t(char *str, size_t *result, const char *variable_name) {
    char *endptr;
    uintmax_t result_umax = strtoumax(str, &endptr, 0);
    if (endptr == str || *endptr != '\0') {
        LOG_ERROR("Invalid %s. \"%s\" is not a valid number of size_t type\n", variable_name, str);
        return -1;
    }
    if (result_umax == UINTMAX_MAX || result_umax > SIZE_MAX) {
        LOG_ERROR("Invalid %s. %s is too large\n", variable_name, str);
        return -1;
    }
    *result = result_umax;
    return 0;
}

int str_to_float(char *str, float *result, const char *variable_name) {
    char *endptr;
    *result = strtof(str, &endptr);
    if (endptr == str || *endptr != '\0') {
        LOG_ERROR("Invalid %s. \"%s\" is not a valid number of float type\n", variable_name, str);
        return -1;
    }
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
int run_experiment(FILE *input_file, size_t init_capacity, float load_factor) {
    if (printf("\nInitial map capacity: %zu\n", init_capacity) < 0) {
        LOG_ERROR("Unable to print initial map capacity\n");
        return -1;
    }
    clock_t clock_before = clock();
    MAP *map = new_MAP(init_capacity, load_factor, simple_hash);
    if (map == NULL) {
        LOG_ERROR("Unable to allocate memory for map\n");
        return -1;
    }
    char word[MAX_WORD_LENGTH];
    while (fread_word(input_file, word, MAX_WORD_LENGTH) != NULL) {
        int *count_ptr = MAP_get(map, word);
        if (count_ptr == NULL) {
            MAP_put(map, word, 1);
            if (MAP_log_and_free_on_error(map)) return -1;
        } else (*count_ptr)++;
    }
    MAP_fprint_stats(map, stdout);
    if (MAP_log_and_free_on_error(map)) return -1;
    ENTRY_ITERATOR *iterator = MAP_get_entry_iterator(map);
    if (MAP_log_and_free_on_error(map)) return -1;
    ENTRY *most_common_word_entry = NULL, *cur_entry;
    while ((cur_entry = ENTRY_ITERATOR_next(iterator)) != NULL)
        if (most_common_word_entry == NULL || ENTRY_get_value(cur_entry) > ENTRY_get_value(most_common_word_entry))
            most_common_word_entry = cur_entry;
    ENTRY_ITERATOR_free(iterator);
    if (MAP_log_and_free_on_error(map)) return -1;
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
    clock_t clock_after = clock();
    if (printf("Time taken: %f seconds\n\n", ((double) (clock_after - clock_before)) / CLOCKS_PER_SEC) < 0) {
        LOG_ERROR("Unable to print execution time\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < MIN_ARG_COUNT) {
        LOG_ERROR("Invalid number of command line arguments. Expected at least %d arg(s), but found %d arg(s)\n"
                  "Usage: word_counter <input file> <load factor> <capacities...>\n",
                  MIN_ARG_COUNT - 1, argc - 1);
        return -1;
    }
    float load_factor;
    if (str_to_float(argv[LOAD_FACTOR_ARG_INDEX], &load_factor, "load factor"))
        return -1;
    FILE *input_file = fopen(argv[INPUT_FILE_ARG_INDEX], "r");
    if (input_file == NULL) {
        LOG_ERROR("Unable to find input file \"%s\"\n", argv[INPUT_FILE_ARG_INDEX]);
        return -1;
    }
    size_t capacity;
    for (int i = FIRST_CAPACITY_ARG_INDEX; i < argc; i++) {
        if (str_to_size_t(argv[i], &capacity, "capacity") != 0
            || run_experiment(input_file, capacity, load_factor) != 0) {
            fclose(input_file);
            return -1;
        }
        rewind(input_file);
    }
    fclose(input_file);
    return 0;
}
