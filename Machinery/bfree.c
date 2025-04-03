/**
 * Ben Williams 
 * March 27th, 2025
 */

#include "bfree.h"
#include "../Datastructures/datastructures.h"
#include "../Headers/headers.h"
#include "../globals.h"

static void free_dynamic_allocation(PBHEAP_BLOCK dynamic_block, void* allocation);

/**
 * Frees a previously bmalloc'd pointer in memory
 */
void bfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    assert(gl_bheap_initialized);

    PBHEAP_BLOCK block = (PBHEAP_BLOCK) DOWN_TO_BLOCK((ULONG_PTR) ptr);

    if (block->block_type == DYNAMIC_BLOCK) {
        free_dynamic_allocation(block, ptr);
        return;
    }

    assert(FALSE);
}


/**
 * Takes the allocation from a dynamic block and frees it - adding it to a respective free list
 * 
 * Note that this current approach could lead to a lot of fragmentation inside of the block as we don't do any consolidation right now
 */
static void free_dynamic_allocation(PBHEAP_BLOCK dynamic_block, void* allocation) {

    PDYNAMIC_ALLOCATION dynamic_allocation = (PDYNAMIC_ALLOCATION) ((ULONG_PTR) allocation - DYNAMIC_ALLOCATION_OVERHEAD);

    dynamic_insert_into_freelist(dynamic_block, dynamic_allocation);
}



