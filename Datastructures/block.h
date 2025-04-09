/**
 * Ben Williams
 * March 22nd, 2025
 */

#include "../Headers/headers.h"

#define DYNAMIC_BLOCK 0
#define UNIFORM_BLOCK 1

#define RED 0
#define BLACK 1

// Example: (16 / 8) - 2 --> 0
#define ALLOCATION_SIZE_TO_INDEX(x) ((x / 8) - 2)

#ifndef DYN_ALLOC
#define DYN_ALLOC

/**
 * Allocations from dynamic blocks need to store metadata about the allocations to handle frees
 */
typedef struct DYNAMIC_ALLOCATION_STRUCT {
    ULONG_PTR allocation_size;
    void* data;
} DYNAMIC_ALLOCATION, *PDYNAMIC_ALLOCATION;

#endif

// We don't count the "data" pointer as overhead, just the size
#define DYNAMIC_ALLOCATION_OVERHEAD (sizeof(DYNAMIC_ALLOCATION) - 8)

#ifndef ALL_BLOCK
#define ALL_BLOCK

/**
 * Used for tracking the links in a free list of allocations in a dynamic block
 */
typedef struct FREED_DYNAMIC_ALLOCATION_STRUCT {
    ULONG_PTR allocation_size;
    struct FREED_DYNAMIC_ALLOCATION_STRUCT* flink;
} FREED_DYNAMIC_ALLOCATION, *PFREED_DYNAMIC_ALLOCATION;


/**
 * The freelist structure for freed allocations in a dynamic block
 */
typedef struct {
    volatile ULONG_PTR list_length;
    volatile WORD lock;
    PFREED_DYNAMIC_ALLOCATION head;
} ALLOCATION_FREELIST, *PALLOCATION_FREELIST;


/**
 * A block of memory for managing dynamic/differently sized allocations
 * 
 * As of right now, we guarantee sizeof(this) + sizeof(BHEAP_BLOCK) to be <= PAGESIZE.
 * This allows us to effectively set aside the first 4kb of the block for all of the metadata.
 */
typedef struct BHEAP_DYNAMIC_BLOCK_STRUCT {
    // This would allow us to expand dynamic blocks' reserved memory and add new extensions
    PULONG_PTR block_reserve_limit;
    ALLOCATION_FREELIST freelists[NUM_ALLOCATION_SIZES];
} BHEAP_DYNAMIC_BLOCK, *PBHEAP_DYNAMIC_BLOCK;


/**
 * A block of memory for managing uniformly sized allocations (all are the same size)
 */
typedef struct BHEAP_UNIFORM_BLOCK_STRUCT {
    struct BHEAP_UNIFORM_BLOCK_STRUCT* flink; // can later be improved to have a tree datastructure
    PWORD allocation_map;
} BHEAP_UNIFORM_BLOCK, *PBHEAP_UNIFORM_BLOCK;


/**
 * A consistent block struct that will make the tree operations simpler.
 * 
 * Stores the metadata for consistent or dynamic allocations, depending on the block_type
 */
typedef struct BHEAP_BLOCK_STRUCT {
    volatile WORD lock;

    volatile ULONG_PTR block_contention_count;

    // The base address where allocations are actually made
    PULONG_PTR block_base;

    // The boundary at which we can continue to allocate into committed memory 
    volatile PULONG_PTR block_wilderness;

    // The limit for committed memory
    volatile PULONG_PTR block_commit_limit;

    WORD block_type:1;
    WORD color:1;
    WORD unused:14;

    // Tree functionality is handled with these pointers and the color above
    struct BHEAP_BLOCK_STRUCT* left;
    struct BHEAP_BLOCK_STRUCT* right;
    struct BHEAP_BLOCK_STRUCT* parent;


    union {
        BHEAP_DYNAMIC_BLOCK dynamic_block;
        BHEAP_UNIFORM_BLOCK uniform_block;
    };
} BHEAP_BLOCK, *PBHEAP_BLOCK;


typedef struct {
    PBHEAP_BLOCK root;
    SRWLOCK tree_lock;
} BHEAP_BLOCK_TREE, *PBHEAP_BLOCK_TREE;

#endif

/**
 * Unlinks an allocation of the given size from the block, if possible.
 * 
 * If this fails (we may run out of entries due to thread collisions, for instance),
 * we return NULL
 */
PDYNAMIC_ALLOCATION dynamic_unlink_from_freelist(PBHEAP_BLOCK block, ULONG_PTR allocation_size);


/**
 * Inserts the given allocation into the appropriate freelist for the block
 * 
 */
void dynamic_insert_into_freelist(PBHEAP_BLOCK block, PDYNAMIC_ALLOCATION allocation);


/**
 * Allocates memory from the dynamic block if possible. 
 * 
 * If there are entries for the given size from a free list, it will take it from that. Otherwise,
 * it will extend from the block's wilderness.
 */
PDYNAMIC_ALLOCATION allocate_from_dynamic_block(PBHEAP_BLOCK block, ULONG_PTR allocation_size);


/**
 * Finds the relevant block for the given address and block tree, if it exists
 */
PBHEAP_BLOCK find_relevant_block(PBHEAP_BLOCK_TREE tree, void* addr);


/**
 * Handles the insertion of the new given block into the tree, while maintaining the red-black balance of the tree
 */
void insert_block_into_tree(PBHEAP_BLOCK_TREE tree, PBHEAP_BLOCK block);


/**
 * Creates a uniform section inside of the block for the given allocation size, and creates the metadata sections for it.
 * 
 * Returns TRUE if successful, FALSE if the block did not have enough space or if committing the memory failed
 */
BOOL create_uniform_section(PBHEAP_BLOCK uniform_block, ULONG_PTR allocation_size);