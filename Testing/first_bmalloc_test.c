/**
 * Ben Williams
 * 4/2/25
 */

#include <Windows.h>
#include "../Machinery/bmalloc.h"
#include <stdio.h>
#include <stdlib.h>

void infinite_loop() {
    volatile ULONG64 count = 0;
    while (TRUE) {
        count++;
    }
}


void first_single_thread_tests() {
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

    // now test some frees
    bfree(allocation_16);
    bfree(allocation_2_16);

    PULONG_PTR re_alloc16 = bmalloc(16);
    PULONG_PTR re_alloc16_2 = bmalloc(16);

    // Order is flipped since we are getting from the head of a freelist
    if (re_alloc16 != allocation_2_16 || re_alloc16_2 != allocation_16) {
        infinite_loop();
    }

    bfree(allocation_128);
    bfree(allocation_2_128);

    PULONG_PTR final_alloc = bmalloc(16);

    if (final_alloc == re_alloc16 || final_alloc == re_alloc16_2 || final_alloc == NULL) {
        infinite_loop();
    }



}


void large_allocation_test_single_thread() {
    PULONG_PTR allocation_ptr;

    allocation_ptr = bmalloc(4096);

    if (allocation_ptr == NULL) {
        printf("Test failed - first allocation is NULL\n");
        infinite_loop();
    }

    *allocation_ptr = 0xFFFFFFFF;

    PULONG_PTR mega_ptr = bmalloc(4096 * 1024);

    if (mega_ptr == NULL) {
        printf("Test failed - much larger allocation is NULL");
    }

    for (char* ptr = mega_ptr; ptr < (char*) mega_ptr + (4096 * 1024); ptr++) {
        *ptr = '\xff';
    }

    bfree(allocation_ptr);
    bfree(mega_ptr);

    printf("Large allocation test single thread succeeded\n");
}


int main(int argc, char** argv) {

    if (argc == 1) {
        first_single_thread_tests();
        return;
    }

    if (argc == 2) {
        large_allocation_test_single_thread();
        return;
    }
    
    // for (int i = 0; i < 100; i++) {
    //     bmalloc(128);
    // }
}