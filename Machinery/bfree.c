/**
 * Ben Williams 
 * March 27th, 2025
 */

#include "bfree.h"
#include "../Datastructures/datastructures.h"
#include "../Headers/headers.h"
#include "../globals.h"

static void free_large_allocation(void* allocation);

static void free_consistent_allocation(PBHEAP_BLOCK uniform_block, void* allocation);

static void free_dynamic_allocation(PBHEAP_BLOCK dynamic_block, void* allocation);


/**
 * Frees a previously bmalloc'd pointer in memory
 */
void bfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    assert(gl_bheap_initialized);

    PBHEAP_BLOCK block = find_relevant_block(&gl_bheap_tree, ptr);

    if (block == NULL) {
        free_large_allocation(ptr);
        return;
    }

    if (block->block_type == DYNAMIC_BLOCK) {
        free_dynamic_allocation(block, ptr);
        return;
    }

    assert(FALSE);
}


/**
 * Handles freeing large, individually VirtualAlloc'd chunks
 */
static void free_large_allocation(void* allocation) {
    MEMORY_BASIC_INFORMATION allocation_info;

    assert(VirtualQuery(allocation, &allocation_info, sizeof(allocation_info)) != 0);

    assert(allocation_info.AllocationBase == allocation_info.BaseAddress && allocation_info.AllocationBase == allocation);

    VirtualFree(allocation, allocation_info.RegionSize, MEM_RELEASE);
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




