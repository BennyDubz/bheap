/**
 * Ben Williams
 * March 24th, 2025
 */

#include <block_operations.h>
#include <../globals.h>


/**
 * Creates a new BHEAP_BLOCK and returns a pointer to it if it is successful,
 * and NULL otherwise.
 * 
 * We MEM_RESERVE BLOCK_SIZE of the address space, but only MEM_COMMIT 2 * PAGESIZE,
 * enough for the metadata and the first allocation block.
 */
PBHEAP_BLOCK create_new_block(WORD block_type) {
    PULONG_PTR block_base = VirtualAlloc2   (NULL, 
                                            NULL,
                                            BLOCK_SIZE,
                                            MEM_RESERVE,
                                            PAGE_READWRITE,
                                            &gl_block_mem_parameters,
                                            1);
    
    if (block_base == NULL) {
        return NULL;
    }

    PBHEAP_BLOCK new_block = VirtualAlloc2  (NULL,
                                            block_base,
                                            DEFAULT_INIT_SIZE,
                                            MEM_COMMIT,
                                            PAGE_READWRITE,
                                            NULL,
                                            0);
    
    if (new_block == NULL) {
        VirtualFree(block_base, BLOCK_SIZE, MEM_RELEASE);
        return NULL;
    }

    new_block->block_size = DEFAULT_INIT_SIZE;
    new_block->remaining_memory = DEFAULT_INIT_SIZE - PAGESIZE;
    new_block->block_type = block_type;
    new_block->block_base = ((char*) (&new_block)) + PAGESIZE;
    new_block->block_limit = ((char*) (&new_block)) + DEFAULT_INIT_SIZE;

    return new_block;
}

