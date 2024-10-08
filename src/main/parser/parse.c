// Lie Leon Alexius

#include "parse.h"
#include "csv_parser.h"
#include "terminal_parser.h"

/**
 * @brief Count total lines in CSV
 * 
 * @details
 * We don't do check if .csv has valid structure, we just count the lines (requests).
 * Each non-empty lines is considered as a request, regardless whether they are valid or not
 * The real check is in csv_parser.c
 * 
 * @note this returns `Number_Of_Request` remember to add `1` for `.we = -1`
 * 
 * @warning Non-empty row will be counted as a line (a request)
 * 
 * @author Lie Leon Alexius
 */
int calculateLines(const char* input_filename) {
    FILE* file = fopen(input_filename, "r");
    if (file == NULL) {
        fprintf(stderr, "In calculateLines - failed to open file: %s\n", input_filename);
        return -1;
    }

    int count = 0; // count should be incremented only if a line contains at least one character
    char ch;
    bool isLineNotEmpty = false;
    while ((ch = fgetc(file)) != EOF) {
        if (ch != '\n') {
            isLineNotEmpty = true; // Current line has content
        } 
        else {
            if (isLineNotEmpty) {
                count++; // Increment count for non-empty line
                isLineNotEmpty = false; // Reset for the next line
            }
        }
    }

    fclose(file);
    return count;
}

/**
 * @brief Parser starts here
 * @author Lie Leon Alexius
 */
Config* start_parse(int argc, char* argv[]) {

    // Parse User Input
    Config* config = parse_user_input(argc, argv);

    // ========================================================================================

    // Parse .csv
    // Get numRequest of the file - Read Warning calculateLines()
    if (!(config->customNumRequest)) {
        int totalRequest = calculateLines(config->input_filename);
        if (totalRequest == -1) {
            free(config);
            config = NULL;
            fprintf(stderr, "Error when calculating total requests in CSV\n");
            exit(EXIT_FAILURE);
        }
        config->numRequests = totalRequest;
    }

    // Allocate (numRequests + 1) for the .we = -1 marker
    config->requests = malloc((config->numRequests + 1) * sizeof(struct Request));

    // check if malloc successful
    if (config->requests == NULL) {
        free(config);
        config = NULL;
        fprintf(stderr, "Error when allocating Request Struct in Config\n");
        exit(EXIT_FAILURE);
    }

    // run parse_csv
    if (parse_csv(config->input_filename, config->requests, config->numRequests, config->customNumRequest) == -1) {
        free(config->requests);
        config->requests = NULL;
        free(config);
        config = NULL;
        fprintf(stderr, "Error when parsing CSV\n");
        exit(EXIT_FAILURE);
    }

    // ========================================================================================

    // return finished Configuration
    return config;
}