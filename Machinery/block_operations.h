/**
 * Ben Williams
 * March 24th, 2025
 */

#include <Headers/headers.h>
#include <Datastructures/datastructures.h>


/**
 * Creates a new BHEAP_BLOCK and returns a pointer to it if it is successful,
 * and NULL otherwise.
 * 
 * We MEM_RESERVE BLOCK_SIZE of the address space, but only MEM_COMMIT 2 * PAGESIZE,
 * enough for the metadata and the first allocation block.
 */
PBHEAP_BLOCK create_new_block(WORD block_type);


/**
 * From the given heap address, find its associated block. This will likely be used
 * in free() so we can quickly determine where the relevant block is.
 * 
 * If the block cannot be found, returns NULL.
 */
PBHEAP_BLOCK get_block_from_address(void* address);