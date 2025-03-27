/**
 * Ben Williams
 * March 22nd, 2025
 */

#include <../Headers/headers.h>

#define DYNAMIC_BLOCK 0
#define UNIFORM_BLOCK 1


// Example: (16 / 8) - 2 --> 0
#define ALLOCATION_SIZE_TO_INDEX(x) ((x / 8) - 2)


/**
 * Allocations from dynamic blocks need to store metadata about the allocations to handle frees
 */
typedef struct DYNAMIC_ALLOCATION_STRUCT {
    ULONG_PTR allocation_size;
    void* data;
} DYNAMIC_ALLOCATION, *PDYNAMIC_ALLOCATION;


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
    volatile SHORT lock;
    PFREED_DYNAMIC_ALLOCATION head;
} ALLOCATION_FREELIST, *PALLOCATION_FREELIST;


/**
 * A block of memory for managing dynamic/differently sized allocations
 * 
 * As of right now, we guarantee sizeof(this) + sizeof(BHEAP_BLOCK) to be <= PAGESIZE.
 * This allows us to effectively set aside the first 4kb of the block for all of the metadata.
 */
typedef struct BHEAP_DYNAMIC_BLOCK_STRUCT {
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
 * A consistent block struct to make the tree operations simpler
 */
typedef struct BHEAP_BLOCK_STRUCT {
    volatile WORD lock;
    WORD block_type;

    // The base address where allocations are actually made
    PULONG_PTR block_base;

    // The boundary at which we can continue to allocate into committed memory 
    PULONG_PTR block_wilderness;

    // The limit for committed memory
    PULONG_PTR block_limit;
     
    ULONG_PTR block_size;
    volatile ULONG64 remaining_memory;
    union {
        BHEAP_DYNAMIC_BLOCK dynamic_block;
        BHEAP_UNIFORM_BLOCK uniform_block;
    };
} BHEAP_BLOCK, *PBHEAP_BLOCK;


/**
 * Unlinks an allocation of the given size from the block, if possible.
 * 
 * If this fails (we may run out of entries due to thread collisions, for instance),
 * we return NULL
 */
PULONG_PTR dynamic_unlink_from_freelist(PBHEAP_BLOCK block, ULONG_PTR allocation_size);


/**
 * Inserts the given allocation into the appropriate freelist for the block
 * 
 */
void dynamic_insert_into_freelist(PBHEAP_BLOCK block, PDYNAMIC_ALLOCATION allocation);