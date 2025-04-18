/**
 * Ben Williams
 * March 22nd, 2025
 */

#include <Windows.h>
#include <Headers/headers.h>
#include <Datastructures/datastructures.h>


/**
 * Variables
 */

extern BOOL gl_bheap_initialized;

extern PULONG_PTR gl_bheap_base;


/**
 * Datastructures
 */
extern BHEAP_STATE gl_bheap_state;

extern BHEAP_BLOCK_TREE gl_bheap_tree;

/**
 * Other
 */

extern MEM_EXTENDED_PARAMETER gl_block_mem_parameters;