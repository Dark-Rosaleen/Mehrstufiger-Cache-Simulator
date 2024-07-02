// Lie Leon Alexius
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>
#include "printer.h"

// Request and Result struct
// #include "../modules/modules.hpp"
#include "placeHolder.hpp"

/**
 * @brief The run_simulation method in C++
 * @author Lie Leon Alexius
 */
extern int run_simulation(
    int cycles,
    unsigned l1CacheLines, unsigned l2CacheLines, unsigned cacheLineSize, 
    unsigned l1CacheLatency, unsigned l2CacheLatency, unsigned memoryLatency, 
    size_t numRequests, struct Request* requests,
    const char* tracefile
);

/**
 * @brief Method for -h or --help
 * @note Shortest possible input is `./cache filename.csv`
 * @warning DO NOT CHANGE THIS
 * @author Lie Leon Alexius
 */
void print_help() {
    printf("Usage: ./cache [OPTIONS] filename.csv\n");
    printf("Options:\n");
    printf("  -c, --cycles <num>           The number of cycles to be simulated (default: 1000000)\n");
    printf("      --cacheline-size <num>   The size of a cache line in bytes (default: 64)\n");
    printf("      --l1-lines <num>         The number of cache lines of the L1 cache (default: 64)\n");
    printf("      --l2-lines <num>         The number of cache lines of the L2 cache (default: 256)\n");
    printf("      --l1-latency <num>       The latency of the L1 cache in cycles (default: 4)\n");
    printf("      --l2-latency <num>       The latency of the L2 cache in cycles (default: 12)\n");
    printf("      --memory-latency <num>   The latency of the main memory in cycles (default: 100)\n");
    printf("      --tf=<filepath>          Output file for a trace file with all signals (default: default_trace.vcd)\n");
    printf("  -h, --help                   Display this help and exit\n");
}

/**
 * @brief removes whitespaces from a string
 * @note `"Le o n"` would be `"Leon"` and `" "` would be `""`
 * @author Lie Leon Alexius
 */
void remove_whitespaces(char* input) {
    int n = strlen(input);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (input[i] != ' ') {
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
}

/**
 * @brief Parses a .csv file and fills the Request struct.
 *
 * @note A valid .csv file follows these rules:
 *  1. A valid row is in the format of `W,Adr,Val` or `R,Adr` or `R,Adr,<whitespace(s)>`.
 *  2. New lines at the end of the file are allowed.
 *  3. New lines between valid rows are allowed.
 *  4. `Adr` can be either decimal or hexadecimal (e.g., 0x123 or 0X123).
 *  5. `Val` can be either decimal or hexadecimal (e.g., 0x123 or 0X123).
 *  6. The number of valid rows must be greater than or equal to the number of requests to be simulated.
 *
 * @param input_filename The name of the .csv file to parse.
 * @param requests An array of Request structs to fill.
 * @param numRequests The number of requests to be simulated.
 *
 * @return void
 *
 * @todo Adjust the maximum buffer length (need to calculate how much we need).
 *
 * @warning DO NOT REMOVE ANY OF THE COMMENTS!
 * @author Lie Leon Alexius
 */
void parse_csv(const char* input_filename, struct Request* requests, int numRequests) {
    // Open the file
    // Syntax: FILE* fptr; fptr = fopen(filename, mode);
    FILE* file = fopen(input_filename, "r");
    if (!file) {
        // No need to close file here since fopen will return NULL
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    // Initialize buffer
    char line[100]; // assume a line up to 100 char (max)
    char rw[10]; // R or W
    char addr_str[20]; // Hexadecimal or Decimal
    char data_str[20]; // Hexadecimal or Decimal

    int i = 0; // request(s) counter

    // Read the requests from the file
    // Syntax: fgets(buffer, buffer_size, fptr);
    /*
        """
        my name is John Doe\n
        my name is Jane Doe\n
        \n
        \n
        """
        fgets() -> "my name is John Doe\n"
        fgets() -> "my name is Jane Doe\n"
        fgets() -> "\n"
        fgets() -> "\n"
    */
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove the first occurrence of (\n) replace it with the null character (\0)
        line[strcspn(line, "\n")] = 0;

        // Reset buffers
        memset(rw, 0, sizeof(rw));
        memset(addr_str, 0, sizeof(addr_str));
        memset(data_str, 0, sizeof(data_str));

        // Parse the line
        // %[^,] matches any sequence of characters except for a comma (stop if see comma)
        // , is a delimiter marks that next field starts after a comma
        int fields = sscanf(line, "%[^,],%[^,],%s", rw, addr_str, data_str);

        /*  Cases
            1. "Hello,World,!" -> "Hello"; "World"; "!"
            2. "He,llo,World,!" -> "He"; "llo"; "World,!" (PROBLEM)
            3. "    " -> "    "; ""; "" (EDGE CASE)
        */

        // Remove whitespace(s)
        remove_whitespaces(rw);
        remove_whitespaces(addr_str);
        remove_whitespaces(data_str);

        // Check if the third field has no other field
        // strchr() returns a pointer to the first occurrence of the character c in the string s
        if (strchr(data_str, ',') != NULL) {
            fprintf(stderr, "Invalid third collumn: %s\n", data_str);
            exit(EXIT_FAILURE);
        }
        
        // Case: trailing whitespace in the data field for read requests
        // Note: remove_whitespaces() should have made 1st char into '\0'
        if (fields == 3 && rw[0] == 'R' && data_str[0] == '\0') {
            fields = 2;
        }

        // Min. valid field in a row = 2
        // Note: This should be triggered only if the syntax of .csv is false
        //       We ignore Empty lines (look at if-else if-else)
        if (fields < 2 && (rw[0] != '\0' || addr_str[0] != '\0' || data_str[0] != '\0')) {
            fprintf(stderr, "Error in parsing the data - wrong format\n");
            exit(EXIT_FAILURE);
        }

        // Case: Write
        if (strcmp(rw, "W") == 0) {
            if (fields != 3) {
                fprintf(stderr, "Error in parsing the data - wrong format for write request\n");
                exit(EXIT_FAILURE);
            }
            requests[i].we = 1;
            
            // Parse address
            if (addr_str[1] == 'x' || addr_str[1] == 'X') {
                sscanf(addr_str, "%x", &requests[i].addr);
            } else {
                sscanf(addr_str, "%u", &requests[i].addr);
            }

            // Parse data
            if (data_str[1] == 'x' || data_str[1] == 'X') {
                sscanf(data_str, "%x", &requests[i].data);
            } else {
                sscanf(data_str, "%u", &requests[i].data);
            }

            i++;
        }

        // Case: Read
        else if (strcmp(rw, "R") == 0) {
            if (fields != 2) {
                fprintf(stderr, "Error in parsing the data - wrong format for read request\n");
                exit(EXIT_FAILURE);
            }

            requests[i].we = 0;
            requests[i].data = 0; // Default value for Read

            // Parse address
            if (addr_str[1] == 'x' || addr_str[1] == 'X') {
                sscanf(addr_str, "%x", &requests[i].addr);
            } else {
                sscanf(addr_str, "%u", &requests[i].addr);
            }

            i++;
        }

        // Case: Unknown Op
        else {
            // ignore empty line with whitespaces (See case-3 in sscanf())
            if(rw[0] == '\0') {
                continue;
            }
            else {
                fprintf(stderr, "Error in parsing the data - unrecognized command\n");
                exit(EXIT_FAILURE);
            }
        }

        // Case: Enough valid Requests has been read
        if (i >= numRequests) {
            break;
        }
    }

    // Close the file
    fclose(file);

    // check if numRequests has been fulfilled
    if (i != numRequests) {
        fprintf(stderr, "Error: number of requests parsed does not match numRequests\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 
 * This function parses the user inputs and set the variables accordingly. 
 * After that, it will calls the simulator to run the simulation.
 * 
 * @details
 * Each of the variables has default values:
 *  1. cycles = 1000000 (default simulation cycles)
 *  2. l1CacheLines = 64 (default number of lines in L1 cache)
 *  3. l2CacheLines = 256 (default number of lines in L2 cache)
 *  4. cacheLineSize = 64 (default cache line size in bytes)
 *  5. l1CacheLatency = 4 (default latency for L1 cache in cycles)
 *  6. l2CacheLatency = 12 (default latency for L2 cache in cycles)
 *  7. memoryLatency = 100 (default latency for memory access in cycles)
 *  8. numRequests = 1000 (default number of requests)
 *  9. tracefile = "default_trace.vcd" (default trace file name) 
 * 
 * @note The main function is the entry point of the program.
 * @link https://d-nb.info/978930487/34 (source for default value)
 * @author Lie Leon Alexius
 */
int main(int argc, char* argv[]) {
    // Default values
    int cycles = 1000000;
    unsigned l1CacheLines = 64;
    unsigned l2CacheLines = 256;
    unsigned cacheLineSize = 64;
    unsigned l1CacheLatency = 4;
    unsigned l2CacheLatency = 12;
    unsigned memoryLatency = 100;
    size_t numRequests = 1000; // Set to n to test .csv
    const char* tracefile = "default_trace.vcd";

    // Filename for the input CSV file
    const char* input_filename = NULL;

    // ==========================================================================================
    // ============================== START PARSING USER INPUT ==================================
    // ==========================================================================================

    // Long options array
    struct option long_options[] = {
        // {const char *name; int has_arg; int *flag; int val;}
        {"cycles", required_argument, 0, 'c'},
        {"cacheline-size", required_argument, 0, 0},
        {"l1-lines", required_argument, 0, 0},
        {"l2-lines", required_argument, 0, 0},
        {"l1-latency", required_argument, 0, 0},
        {"l2-latency", required_argument, 0, 0},
        {"memory-latency", required_argument, 0, 0},
        {"tf", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt; // option flag input
    int long_index = 0; // the index for long_options 
    char* endptr; // parsing error checker

    // (arg count, arg array, legitimate option characters, long options, long options index)
    // https://linux.die.net/man/3/getopt_long
    while ((opt = getopt_long(argc, argv, "c:h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'c':
                errno = 0;
                cycles = strtol(optarg, &endptr, 10);

                // Check for errors during conversion then print it
                if (errno != 0 || *endptr != '\0') {
                    fprintf(stderr, "Invalid input for cycles\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 0:
                if (strcmp("cacheline-size", long_options[long_index].name) == 0) {
                    errno = 0;
                    cacheLineSize = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for cacheline-size\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("l1-lines", long_options[long_index].name) == 0) {
                    errno = 0;
                    l1CacheLines = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for l1-lines\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("l2-lines", long_options[long_index].name) == 0) {
                    errno = 0;
                    l2CacheLines = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for l2-lines\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("l1-latency", long_options[long_index].name) == 0) {
                    errno = 0;
                    l1CacheLatency = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for l1-latency\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("l2-latency", long_options[long_index].name) == 0) {
                    errno = 0;
                    l2CacheLatency = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for l2-latency\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("memory-latency", long_options[long_index].name) == 0) {
                    errno = 0;
                    memoryLatency = strtoul(optarg, &endptr, 10);

                    // Check for errors during conversion then print it
                    if (errno != 0 || *endptr != '\0') {
                        fprintf(stderr, "Invalid input for memory-latency\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else if (strcmp("tf", long_options[long_index].name) == 0) {
                    tracefile = optarg;
                }
                break;
            default:
                // if flag is undefined, print_help then exit
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    // Get the remaining argument: the input filename
    // The variable optind is the index of the next element to be processed in argv.
    // https://linux.die.net/man/3/optind
    if (optind < argc) {
        input_filename = argv[optind];
    } 
    else {
        fprintf(stderr, "Input filename is required\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    // Error checking: Validate input_filename
    // check if NULL
    if (input_filename == NULL) {
        fprintf(stderr, "Input filename is missing\n");
        print_help();
        exit(EXIT_FAILURE);
    }
    
    // Check if the filename ends with .csv
    size_t len = strlen(input_filename);
    if (len <= 4 || strcmp(input_filename + len - 4, ".csv") != 0) {
        fprintf(stderr, "Invalid filename. Filename should end with .csv\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    // ==========================================================================================
    // =============================== END PARSING USER INPUT ===================================
    // ==========================================================================================

    // ==========================================================================================
    // ================================= START PARSING CSV ======================================
    // ==========================================================================================

    /*
        CSV Data (Strict rule):
        1. The 1st Column = R || W
        2. The 2nd Column = Address (either Hexadecimal or Decimal)
        3. The 3rd Column = Value (empty if R or some(v) if W)

        Exception:
        Throws Exc at stderr if there are any deviation from the rules
    */

    // initialize the struct
    struct Request requests[numRequests];

    // parse the csv
    parse_csv(input_filename, requests, numRequests);

    // ==========================================================================================
    // ================================== END PARSING CSV =======================================
    // ==========================================================================================

    // EXPERIMENT
    print_layout();

    // run simulation
    int result = 
    run_simulation(
        cycles, 
        l1CacheLines, l2CacheLines, cacheLineSize, 
        l1CacheLatency, l2CacheLatency, memoryLatency, 
        numRequests, requests, 
        tracefile
    );

    // Process the simulation result
    if (result == 0) {
        printf("Success\n");
    } else {
        printf("Something is not right!\n");
    }

    return 0;
}
