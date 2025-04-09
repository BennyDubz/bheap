/**
 * Ben Williams
 * March 24th, 2025
 */

#include "../Headers/headers.h"
#include "../Datastructures/datastructures.h"


/**
 * Creates a new BHEAP_BLOCK and returns a pointer to it if it is successful,
 * and NULL otherwise.
 * 
 * We MEM_RESERVE BLOCK_SIZE of the address space, but only MEM_COMMIT 2 * PAGESIZE,
 * enough for the metadata and the first allocation block.
 */
PBHEAP_BLOCK create_new_block(WORD block_type);


/**
 * Finds the appropriate dynamic block for the calling thread to use.
 * 
 * This might be a block local to the thread, several threads, or globally to the process
 * depending on how much contention there is. That is the goal, at least down the road.
 */
PBHEAP_BLOCK get_relevant_dynamic_block();


/**
 * Creates a section in the (currently global) consistent block for the given allocation size
 */
void create_uniform_block_section(ULONG_PTR allocation_size);
