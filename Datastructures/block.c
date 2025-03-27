/**
 * Ben Williams
 * March 24th, 2025
 */


#include <block.h>
#include <assert.h>



/**
 * Unlinks an allocation of the given size from the block, if possible.
 * 
 * If this fails (we may run out of entries due to thread collisions, for instance),
 * we return NULL
 */
PULONG_PTR dynamic_unlink_from_freelist(PBHEAP_BLOCK block, ULONG_PTR allocation_size) {

    PALLOCATION_FREELIST freelist = &block->dynamic_block.freelists[ALLOCATION_SIZE_TO_INDEX(allocation_size)];

    #if DEBUG_MODE
    assert(block->block_type == DYNAMIC_BLOCK);
    assert(allocation_size <= MAX_SMALL_ALLOCATION);
    #endif

    // // Try to claim an entry
    // if (InterlockedDecrement64(&block->dynamic_block.freelists[freelist_index].list_length) < 0) {
    //     InterlockedIncrement64(&block->dynamic_block.freelists[freelist_index].list_length);
    //     return NULL;
    // } 

    while (InterlockedCompareExchangeAcquire16(&freelist->lock, SPINLOCK_LOCKED, SPINLOCK_UNLOCKED) == SPINLOCK_LOCKED) {
        if (freelist->list_length == 0) {
            return NULL;
        }
    }

    PFREED_DYNAMIC_ALLOCATION to_allocate = freelist->head;

    freelist->head = to_allocate->flink;

    InterlockedDecrement64(&freelist->list_length);
    InterlockedDecrementRelease16(&freelist->lock);

    return to_allocate;
}


/**
 * Inserts the given allocation into the appropriate freelist for the block
 * 
 */
void dynamic_insert_into_freelist(PBHEAP_BLOCK block, PDYNAMIC_ALLOCATION allocation) {
    PALLOCATION_FREELIST freelist = &block->dynamic_block.freelists[ALLOCATION_SIZE_TO_INDEX(allocation->allocation_size)];
    PFREED_DYNAMIC_ALLOCATION recast = (PFREED_DYNAMIC_ALLOCATION) allocation;

    #if DEBUG_MODE
    assert(block->block_type == DYNAMIC_BLOCK);
    assert(allocation->allocation_size <= MAX_SMALL_ALLOCATION);
    #endif

    while (InterlockedCompareExchangeAcquire16(&freelist->lock, SPINLOCK_LOCKED, SPINLOCK_UNLOCKED) == SPINLOCK_LOCKED) {
        YieldProcessor();
    }

    recast->flink = freelist->head;

    freelist->head = recast;
    
    InterlockedIncrement64(&freelist->list_length);
    InterlockedDecrementRelease16(&freelist->lock);
}