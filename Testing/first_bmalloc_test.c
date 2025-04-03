/**
 * Ben Williams
 * 4/2/25
 */

#include <Windows.h>
#include "../Machinery/bmalloc.h"

void infinite_loop() {
    volatile ULONG64 count = 0;
    while (TRUE) {
        count++;
    }
}

int main(int argc, char** argv) {

    PULONG_PTR allocation_16;
    PULONG_PTR allocation_2_16;
    PULONG_PTR allocation_128;
    PULONG_PTR allocation_2_128;

    allocation_16 = bmalloc(16);

    if (allocation_16 == NULL) {
        infinite_loop();
    }

    *allocation_16 = 0xFFFFFFFF;

    allocation_2_16 = bmalloc(16);

    if (allocation_2_16 == NULL || allocation_2_16 == allocation_16) {
        infinite_loop();
    }

    // 16 bytes + 8 byte overhead
    if (allocation_16 + 3 != allocation_2_16) {
        infinite_loop();
    }

    allocation_128 = bmalloc(128);
    allocation_2_128 = bmalloc(128);

    // (8 * 16) bytes + 8 byte overhead
    if (allocation_128 + 17 != allocation_2_128) {
        infinite_loop();
    }

    int i = 16;
    while (bmalloc(i) != NULL) {
        i += 8;
    }

    // for (int i = 0; i < 100; i++) {
    //     bmalloc(128);
    // }
}