/**
 * Ben Williams
 * March 24th, 2025
 */


#include "block.h"
#include <memoryapi.h>
#include <assert.h>
#include <stdio.h>



/**
 * Unlinks an allocation of the given size from the block, if possible.
 * 
 * If this fails (we may run out of entries due to thread collisions, for instance),
 * we return NULL
 */
PDYNAMIC_ALLOCATION dynamic_unlink_from_freelist(PBHEAP_BLOCK block, ULONG_PTR allocation_size) {

    PALLOCATION_FREELIST freelist = &block->dynamic_block.freelists[ALLOCATION_SIZE_TO_INDEX(allocation_size)];

    #if DEBUG_MODE
    assert(block->block_type == DYNAMIC_BLOCK);
    assert(allocation_size <= MAX_SMALL_ALLOCATION);
    #endif


    while (InterlockedCompareExchangeAcquire16(&freelist->lock, SPINLOCK_LOCKED, SPINLOCK_UNLOCKED) == SPINLOCK_LOCKED) {
        if (freelist->list_length == 0) {
            return NULL;
        }
    }

    PFREED_DYNAMIC_ALLOCATION to_allocate = freelist->head;

    freelist->head = to_allocate->flink;

    InterlockedDecrement64(&freelist->list_length);
    InterlockedDecrementRelease16(&freelist->lock);

    return (PDYNAMIC_ALLOCATION) to_allocate;
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


/**
 * Spins until we are able to acquire the lock for the heap block
 */
static void acquire_block_lock(PBHEAP_BLOCK block) {
    while (InterlockedCompareExchangeAcquire16(&block->lock, SPINLOCK_LOCKED, SPINLOCK_UNLOCKED) == SPINLOCK_LOCKED) {
        InterlockedIncrement64(&block->contention_count);
        YieldProcessor();
    }
 }


/**
 * Releases the block's spinlock
 */
static void release_block_lock(PBHEAP_BLOCK block) {
    #if DEBUG_MODE
    assert(block->lock == SPINLOCK_LOCKED);
    #endif

    InterlockedDecrementRelease16(&block->lock);
}


/**
 * Expand's the block's wilderness by committing more memory as necessary. We expand at least to the min_expansion
 * address if possible
 */
static BOOL expand_block_commit(PBHEAP_BLOCK block, PULONG_PTR min_expansion) {

    // See if we've reached our limit of reserved memory
    // Later I would like to be able to expand a block's reserved memory as well (mainly for dynamic blocks)
    if (block->block_commit_limit == block->dynamic_block.block_reserve_limit) {
        return FALSE;
    }

    PULONG_PTR original_limit = block->block_commit_limit;

    acquire_block_lock(block);

    // Another thread already expanded the block, we might not need to do anything
    if (original_limit != block->block_commit_limit && block->block_commit_limit > min_expansion) {
        release_block_lock(block);
        return TRUE;
    }

    ULONG_PTR block_size = (ULONG_PTR) block->block_commit_limit - (ULONG_PTR) block->block_base;
    
    ULONG_PTR expansion_size = ROUND_UP_PAGESIZE(block_size / 2);

    // Determine expansion size - double the amount already allocated until we hit the reserve limit
    if ((ULONG_PTR) block->block_base + expansion_size > (ULONG_PTR) block->dynamic_block.block_reserve_limit) {
        expansion_size = (ULONG_PTR) block->dynamic_block.block_reserve_limit - (ULONG_PTR) block_size;
    }

    // Commit more memory
    PULONG_PTR expansion_ptr = VirtualAlloc2(NULL,
                                            block->block_commit_limit,
                                            expansion_size,
                                            MEM_COMMIT,
                                            PAGE_READWRITE,
                                            NULL,
                                            0);

    if (expansion_ptr == NULL) {
        release_block_lock(block);
        return FALSE;
    }

    block->block_commit_limit = (PULONG_PTR) ((ULONG_PTR) block->block_commit_limit + expansion_size);

    #if DEBUG_MODE
    printf("Expanded commit limit by %llx from %p to %p\n", expansion_size, (PULONG_PTR) original_limit, block->block_commit_limit);
    #endif


    release_block_lock(block);

    return TRUE;
}


/**
 * Allocates memory from the dynamic block if possible. 
 * 
 * If there are entries for the given size from a free list, it will take it from that. Otherwise,
 * it will extend from the block's wilderness.
 */
PDYNAMIC_ALLOCATION allocate_from_dynamic_block(PBHEAP_BLOCK block, ULONG_PTR allocation_size) {

    // Try to get it from a free list first, if it exists
    if (block->dynamic_block.freelists[ALLOCATION_SIZE_TO_INDEX(allocation_size)].head != NULL) {
        PDYNAMIC_ALLOCATION possible_result = dynamic_unlink_from_freelist(block, allocation_size);

        if (possible_result != NULL) {
            return possible_result;
        }
    }

    ULONG_PTR true_allocation_size = allocation_size + DYNAMIC_ALLOCATION_OVERHEAD;

    PULONG_PTR allocation_addr_end;

    if ((allocation_addr_end = (PULONG_PTR) InterlockedAdd64((PULONG64) &block->block_wilderness, true_allocation_size)) 
            > block->block_commit_limit) {
            
        // Expand the block at least to cover our current allocation, but likely more
        if (expand_block_commit(block, allocation_addr_end) == FALSE) {
            // Undo our change to the block wilderness
            InterlockedAdd64((PULONG64) &block->block_wilderness, - true_allocation_size);
            return NULL;
        }
    }

    PDYNAMIC_ALLOCATION recast = (PDYNAMIC_ALLOCATION) ((ULONG_PTR) allocation_addr_end - allocation_size);

    recast->allocation_size = allocation_size;

    return recast;
}