/**
 * Ben Williams
 * March 27th, 2025
 */

#include "../Datastructures/datastructures.h"
#include "../Headers/headers.h"
#include "init.h"
#include "block_operations.h"
#include "../globals.h"
#include <assert.h>



void* handle_large_request(ULONG_PTR allocation_size);
void* handle_dynamic_request(ULONG_PTR allocation_size);
void* handle_consistent_request(ULONG_PTR allocation_size);

/**
 * Handles all incoming allocation requests
 */
void* bmalloc(ULONG_PTR allocation_size) {

    if (gl_bheap_initialized == FALSE) {
        assert(init_bheap() == SUCCESS);
    }

    PULONG_PTR result;

    if (allocation_size < MIN_ALLOCATION_SIZE) {
        allocation_size = MIN_ALLOCATION_SIZE;
    }

    // Ensure it is 8-byte aligned
    allocation_size = ROUND_UP_SIZE(allocation_size);

    // If this is larger than our standard blocks
    if (allocation_size > MAX_SMALL_ALLOCATION) {
        return handle_large_request(allocation_size);
    }

    // See if we have anything in a consistent block for this allocation size
    if (gl_bheap_state.consistent_blocks_exist[ALLOCATION_SIZE_TO_INDEX(allocation_size)]) {
        result = handle_consistent_request(allocation_size);

        if (result != NULL) {
            return result;
        }
    }

    // Allocate memory from a dynamic block
    return handle_dynamic_request(allocation_size);
}


/**
 * Handles all requests that are > MAX_SMALL_ALLOCATION in size
 */
void* handle_large_request(ULONG_PTR allocation_size) {
    /**
     * 1. VirtualAlloc a sufficiently large amount
     * 
     * 2. Write in or otherwise track the metadata (not sure how yet) to be able to unmap it
     */
    return NULL;
}


/**
 * Handles all requests that will rely on our dynamic block(s) to handle them.
 * 
 */
void* handle_dynamic_request(ULONG_PTR allocation_size) {
    PBHEAP_BLOCK dynamic_block = get_relevant_dynamic_block();

    PDYNAMIC_ALLOCATION dynamic_alloc = allocate_from_dynamic_block(dynamic_block, allocation_size);

    if (dynamic_alloc == NULL) {
        return NULL;
    }

    return &dynamic_alloc->data;
}


/**
 * Handle all requests that will rely on their own sections in a consistent block to handle them
 */
void* handle_consistent_request(ULONG_PTR allocation_size) {
    return NULL;
}
