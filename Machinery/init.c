/**
 * Ben Williams
 * March 22nd, 2025
 */

#include <../Headers/globals.h>
#include <../Headers/macros.h>
#include <../Headers/parameters.h>

ULONG_PTR bheap_base;

ULONG_PTR init_bheap() {
    
    bheap_base = VirtualAlloc   (NULL, 
                                DEFAULT_INIT_SIZE,
                                MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
    
    if (bheap_base == NULL) {
        printf("Failed to initialize heap\n");
        return ERROR;
    }

    
}