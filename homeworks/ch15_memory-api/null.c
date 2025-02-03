#include <stdio.h>

int main() {
    int* ptr = NULL;
    printf("Value at ptr: %d\n", *ptr); // this will crash
    return 0;
}


