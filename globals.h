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

extern BOOL gl_bheap_initialized = FALSE;

extern ULONG_PTR gl_bheap_base;


/**
 * Datastructures
 */
extern BHEAP_STATE gl_bheap_state;


/**
 * Other
 */

extern MEM_EXTENDED_PARAMETER gl_block_mem_parameters;