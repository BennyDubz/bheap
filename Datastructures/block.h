/**
 * Ben Williams
 * March 22nd, 2025
 */

#include <../Headers/headers.h>


/**
 * A block of memory for managing dynamic/differently sized allocations
 */
typedef struct BHEAP_DYNAMIC_BLOCK_STRUCT {
    volatile WORD lock;
    

} BHEAP_DYNAMIC_BLOCK, *PBHEAP_DYNAMIC_BLOCK;


/**
 * A block of memory for managing consistently sized allocations
 */
typedef struct BHEAP_CONSISTENT_BLOCK_STRUCT {
    volatile WORD lock;
    PULONG_PTR block_size;
    volatile PULONG_PTR remaining_memory;
    PWORD allocation_map;
    PULONG_PTR block_base;
} BHEAP_CONSISTENT_BLOCK, *PBHEAP_CONSISTENT_BLOCK;