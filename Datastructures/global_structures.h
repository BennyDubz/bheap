/**
 * Ben Williams
 * March 24th, 2025
 */


#include <../Headers/headers.h>
#include <block.h>


/**
 * This will keep track of the global state of the heap
 */
typedef struct BHEAP_STATE_STRUCT {
    SHORT consistent_blocks_exist[NUM_ALLOCATION_SIZES];
    ULONG64 allocation_counts[NUM_ALLOCATION_SIZES];
    PBHEAP_DYNAMIC_BLOCK dynamic_blocks;

} BHEAP_STATE, *PBHEAP_STATE;