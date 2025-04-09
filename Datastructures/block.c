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
        InterlockedIncrement64(&block->block_contention_count);
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


/**
 * Handles the recursive searching within the tree
 */
static PBHEAP_BLOCK find_relevant_block_helper(PBHEAP_BLOCK node, PULONG_PTR addr) {
    if (node == NULL) {
        return NULL;
    }

    if (addr > node->block_base && addr < node->block_commit_limit) {
        return node;
    }

    if (addr < node->block_base) {
        return find_relevant_block_helper(node->left, addr);
    }

    return find_relevant_block_helper(node->right, addr);
}


/**
 * Finds the relevant block for the given address, if it exists
 */
PBHEAP_BLOCK find_relevant_block(PBHEAP_BLOCK_TREE tree, void* addr) {
    AcquireSRWLockShared(&tree->tree_lock);

    PBHEAP_BLOCK result = find_relevant_block_helper(tree->root, (PULONG_PTR) addr);

    ReleaseSRWLockShared(&tree->tree_lock);

    return result;
}


/**
 * Inserts the block into the tree as if it were a standard binary search tree, without any acknowledgement to the colors
 * 
 * Assumes the tree's lock is held **exclusive** prior to this happening
 */
static void insert_tree_no_rebalance(PBHEAP_BLOCK_TREE tree, PBHEAP_BLOCK block) {
    PBHEAP_BLOCK curr_node = tree->root;

    while (TRUE) {
        // We sort purely based on the addresses of the blocks
        if (curr_node > block) {
            if (curr_node->left) {
                curr_node = curr_node->left;
                continue;
            }

            curr_node->left = block;
            break;
        }

        if (curr_node < block) {
            if (curr_node->right) {
                curr_node = curr_node->right;
                continue;
            }

            curr_node->right = block;
            break;
        }
    }

    block->parent = curr_node;
}


/**
 * Rotates the subtree to the left with block as its "subroot"
 */
static void rotate_left(PBHEAP_BLOCK_TREE tree, PBHEAP_BLOCK block) {
    PBHEAP_BLOCK rotation_partner = block->right;
    
    block->right = rotation_partner->left;

    if (rotation_partner->left != NULL) {
        rotation_partner->left->parent = block;
    }

    rotation_partner->parent = block->parent;

    if (block->parent == NULL) {
        tree->root = rotation_partner;
    } else {
        if (block == block->parent->left) {
            block->parent->left = rotation_partner;
        } else {
            block->parent->right = rotation_partner;
        }
    }

    rotation_partner->left = block;
    block->parent = rotation_partner;
}

/**
 * Rotates the subtree to the right with block as its "subroot"
 */
static void rotate_right(PBHEAP_BLOCK_TREE tree, PBHEAP_BLOCK block) {
    PBHEAP_BLOCK rotation_partner = block->left;

    block->left = rotation_partner->right;

    if (rotation_partner->right != NULL) {
        rotation_partner->right->parent = block;
    }

    rotation_partner->parent = block->parent;

    if (block->parent == NULL) {
        tree->root = rotation_partner;
    } else {
        if (block == block->parent->left) {
            block->parent->left = rotation_partner;
        } else {
            block->parent->right = rotation_partner;
        }
    }

    rotation_partner->right = block;
    block->parent = rotation_partner;
}


/**
 * Handles the insertion of the new given block into the tree, while maintaining the red-black balance of the tree
 * 
 */
void insert_block_into_tree(PBHEAP_BLOCK_TREE tree, PBHEAP_BLOCK block) {
    AcquireSRWLockExclusive(&tree->tree_lock);
    
    insert_tree_no_rebalance(tree, block);

    block->color = RED;
    block->left = NULL;
    block->right = NULL;

    PBHEAP_BLOCK curr_node = block;
    PBHEAP_BLOCK uncle_node;

    while (curr_node != tree->root && curr_node->parent->color == RED) {
        // If curr_node's parent is left, we want the uncle to be a right
        // Since curr_node->parent is RED, it must have a BLACK parent
        if (curr_node->parent == curr_node->parent->parent->left) {
            uncle_node = curr_node->parent->parent->right;

            // If the uncle is red, we can change the parent and uncle and move up the tree
            if (uncle_node->color == RED) {
                curr_node->parent->color = BLACK;
                uncle_node->color = BLACK;
                curr_node = curr_node->parent->parent;
                continue;
            }

            // If the uncle is black, and our current node is a right, we have to move
            // up the tree and rotate left before continuing
            if (curr_node == curr_node->parent->right) {
                curr_node = curr_node->parent;
                rotate_left(tree, curr_node);
                continue;
            }

            // If the uncle is black, and our current node is a left, we have to adjust our parent
            // and grandparent before rotating right
            curr_node->parent->color = BLACK;
            curr_node->parent->parent->color = RED;
            rotate_right(tree, curr_node->parent->parent);

        } else {
            // curr_node's parent is a right, the uncle needs to be a left
            uncle_node = curr_node->parent->parent->left;

            if (uncle_node->color == RED) {
                curr_node->parent->color = BLACK;
                uncle_node->color = BLACK;
                curr_node = curr_node->parent->parent;
                continue;
            }

            if (curr_node == curr_node->parent->right) {
                curr_node = curr_node->parent;
                rotate_left(tree, curr_node);
                continue;
            }

            curr_node->parent->color = BLACK;
            curr_node->parent->parent->color = RED;
            rotate_right(tree, curr_node->parent->parent);
        }
    }

    tree->root->color = BLACK;

    ReleaseSRWLockExclusive(&tree->tree_lock);
}



/**
 * Creates a uniform section inside of the block for the given allocation size, and creates the metadata sections for it.
 * 
 * Returns TRUE if successful, FALSE if the block did not have enough space or if committing the memory failed
 */
BOOL create_uniform_section(PBHEAP_BLOCK uniform_block, ULONG_PTR allocation_size);