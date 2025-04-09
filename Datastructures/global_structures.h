/**
 * Ben Williams
 * March 24th, 2025
 */


#include "../Headers/headers.h"
#include "block.h"

#ifndef GLOBAL_STRUCTS
#define GLOBAL_STRUCTS
/**
 * This will keep track of the global state of the heap
 */
typedef struct BHEAP_STATE_STRUCT {
    CHAR uniform_blocks_exist[NUM_ALLOCATION_SIZES];
    ULONG64 allocation_counts[NUM_ALLOCATION_SIZES];
    PBHEAP_BLOCK dynamic_block;
    PBHEAP_BLOCK uniform_block;
} BHEAP_STATE, *PBHEAP_STATE;

#endif