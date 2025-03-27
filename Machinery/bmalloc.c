/**
 * Ben Williams
 * March 27th, 2025
 */

#include <../Datastructures/datastructures.h>
#include <../Headers/headers.h>
#include <../globals.h>


PULONG_PTR handle_large_request(ULONG_PTR allocation_size);
PULONG_PTR handle_dynamic_request(ULONG_PTR allocation_size);
PULONG_PTR handle_consistent_request(ULONG_PTR allocation_size);

/**
 * Handles all incoming allocation requests
 */
PULONG_PTR bmalloc(ULONG_PTR allocation_size) {
    PULONG_PTR result;

    if (allocation_size < MIN_ALLOCATION_SIZE) {
        allocation_size = MIN_ALLOCATION_SIZE;
    }

    if (allocation_size > MAX_SMALL_ALLOCATION) {
        return handle_large_request(allocation_size);
    }

    if (gl_bheap_state.consistent_blocks_exist[ALLOCATION_SIZE_TO_INDEX(allocation_size)]) {
        result = handle_consistent_request(allocation_size);

        if (result != NULL) {
            return result;
        }
    }

    return handle_dynamic_request(allocation_size);
}


/**
 * Handles all requests that are > MAX_SMALL_ALLOCATION in size
 */
PULONG_PTR handle_large_request(ULONG_PTR allocation_size) {
    /**
     * 1. VirtualAlloc a sufficiently large amount
     * 
     * 2. Write in or otherwise track the metadata (not sure how yet) to be able to unmap it
     */
}


/**
 * Handles all requests that will rely on our dynamic block(s) to handle them.
 * 
 */
PULONG_PTR handle_dynamic_request(ULONG_PTR allocation_size);


/**
 * Handle all requests that will rely on their own sections in a consistent block to handle them
 */
PULONG_PTR handle_consistent_request(ULONG_PTR allocation_size);
