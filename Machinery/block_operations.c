/**
 * Ben Williams
 * March 24th, 2025
 */

#include "block_operations.h"
#include <memoryapi.h>
#include "../globals.h"


/**
 * Creates a new BHEAP_BLOCK and returns a pointer to it if it is successful,
 * and NULL otherwise.
 * 
 * We MEM_RESERVE BLOCK_SIZE of the address space, but only MEM_COMMIT 2 * PAGESIZE,
 * enough for the metadata and the first allocation block.
 */
PBHEAP_BLOCK create_new_block(WORD block_type) {
    PULONG_PTR reserve_block = VirtualAlloc2   (NULL, 
                                            NULL,
                                            BLOCK_SIZE,
                                            MEM_RESERVE,
                                            PAGE_READWRITE,
                                            &gl_block_mem_parameters,
                                            1);
    
    if (reserve_block == NULL) {
        return NULL;
    }

    PBHEAP_BLOCK new_block = VirtualAlloc2  (NULL,
                                            reserve_block,
                                            DEFAULT_INIT_SIZE,
                                            MEM_COMMIT,
                                            PAGE_READWRITE,
                                            NULL,
                                            0);
    
    if (new_block == NULL) {
        VirtualFree(reserve_block, BLOCK_SIZE, MEM_RELEASE);
        return NULL;
    }

    new_block->block_type = block_type;
    new_block->block_base = (PULONG_PTR) (((char*) (new_block)) + PAGESIZE);
    new_block->block_wilderness = new_block->block_base;
    new_block->block_commit_limit = (PULONG_PTR) ((ULONG_PTR) new_block + DEFAULT_INIT_SIZE);

    return new_block;
}


/**
 * Finds the appropriate dynamic block for the calling thread to use.
 * 
 * This might be a block local to the thread, several threads, or globally to the process
 * depending on how much contention there is. That is the goal, at least down the road.
 */
PBHEAP_BLOCK get_relevant_dynamic_block() {
    return gl_bheap_state.dynamic_block;
}


/**
 * Creates a section in the (currently global) consistent block for the given allocation size
 */
void create_uniform_block_section(ULONG_PTR allocation_size) {

    if (gl_bheap_state.uniform_block == NULL) {
        gl_bheap_state.uniform_block = create_new_block(UNIFORM_BLOCK);

        if (gl_bheap_state.uniform_block == NULL) {
            return;
        }
    }

    


}