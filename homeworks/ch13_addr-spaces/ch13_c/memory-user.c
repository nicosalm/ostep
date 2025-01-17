#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Program that allocates and continuously accesses a specified amount of memory
*/
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <memory in MB>\n", argv[0]);
        return 1;
    }

    int megabytes = atoi(argv[1]);
    if (megabytes <= 0) {
        fprintf(stderr, "Positive amount of MBs only");
        return 1;
    }

    // arr size = 1MB = 1024 * 1024 bytes
    size_t size = megabytes * 1024 * 1024;
    int* arr = malloc(size);

    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed and returned NULL ptr");
        return 1;
    }

    // ctnsly stream through arr
    while (1) {
        for (size_t i = 0; i < size/sizeof(int); i++) {
            arr[i] = i;
        }
    }
}
