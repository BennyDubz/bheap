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

    // Uniform blocks only need the metadata page, dynamic blocks need more
    ULONG_PTR commit_size = (block_type == UNIFORM_BLOCK) ? PAGESIZE : DEFAULT_INIT_SIZE;

    PBHEAP_BLOCK new_block = VirtualAlloc2  (NULL,
                                            reserve_block,
                                            commit_size,
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

    // Reserve the first page since we are using it for metadata
    if (block_type == UNIFORM_BLOCK) {
        new_block->uniform_block.allocation_map[0] = 1;
    }

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
 * Commits the pages in the uniform section. Assumes that the bitmap has already been updated.
 * 
 * Returns TRUE upon success, FALSE otherwise
 */
static BOOL commit_uniform_section(PBHEAP_BLOCK block, ULONG_PTR index, ULONG_PTR num_pages) {

    PULONG_PTR commit_base = (PULONG_PTR) ((ULONG_PTR) block + (index * PAGESIZE));

    return VirtualAlloc(commit_base, PAGESIZE * num_pages, MEM_COMMIT, PAGE_READWRITE) != NULL;

}


/**
 * Allocates section_size pages for a uniform section in the heap block, if that many open sequential pages can be found.
 * 
 * Modifies the bitmap and then MEM_COMMITs the pages. 
 */
PUNIFORM_SECTION allocate_uniform_section(PBHEAP_BLOCK block, ULONG_PTR section_size) {
    WORD curr_word;
    WORD alloc_check;
    ULONG_PTR index = 0;
    PWORD end_address = block->uniform_block.allocation_map + UNIFORM_MAP_SIZE;

    for (volatile PWORD alloc_info = block->uniform_block.allocation_map; alloc_info < end_address; alloc_info++) {
        curr_word = *alloc_info;
        alloc_check = SET_LOWEST_NUMBITS(section_size);

        // Loop through all possible times the alloc_check bits can fit inside curr_word
        for (int i = 0; i < sizeof(WORD) - section_size + 1; i++) {
            if (alloc_check & curr_word == 0) {
                // Now actually try to set the bits
                if (InterlockedCompareExchange16(alloc_info, curr_word | alloc_check, curr_word) == curr_word) {
                    
                    if (commit_uniform_section(block, index, section_size) == FALSE) {
                        InterlockedAnd16(alloc_info, ~alloc_check);
                        return NULL;
                    }

                    return (PUNIFORM_SECTION) ((ULONG_PTR) block + (index * PAGESIZE));
                }
            }

            index++;
            alloc_check = alloc_check << 1;
        }
    }

    return NULL;
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

    // To be made more clever/dynamic one day... 
    ULONG_PTR num_pages = 2;
    PUNIFORM_SECTION section = allocate_uniform_section(gl_bheap_state.uniform_block, num_pages);

    if (section == NULL) {
        return;
    }

    section->section_allocation_size = allocation_size;

    // Base value
    ULONG_PTR num_slots = ((num_pages * PAGESIZE) - UNIFORM_SECTION_METADATA_OVERHEAD) / allocation_size;
    // Adjust for how many WORDs we will need for the allocation map
    ULONG_PTR map_overhead = (num_slots / (sizeof(WORD) * 8));
    num_slots = num_slots - map_overhead;
    section->num_remaining_allocations = num_slots;
    section->section_base = (PVOID) ((ULONG_PTR) section + (UNIFORM_SECTION_METADATA_OVERHEAD * 8) + (map_overhead * 8));

    // There are extra slots that we need to mark as used
    if (num_slots % (sizeof(WORD) * 8) > 0) {
        ULONG_PTR extra_slots = (sizeof(WORD) * 8) - (num_slots % (sizeof(WORD) * 8));
        WORD mask = SET_LOWEST_NUMBITS(extra_slots);
        // Shift to the high bits
        mask = mask << (num_slots % (sizeof(WORD) * 8));
        section->allocation_map[(num_slots - 1) / (sizeof(WORD) * 8)] |= mask;
    }

    if (gl_bheap_state.uniform_blocks_exist[ALLOCATION_SIZE_TO_INDEX(allocation_size)] == 0) {
        gl_bheap_state.uniform_blocks_exist[ALLOCATION_SIZE_TO_INDEX(allocation_size)] = 1;
    }

    acquire_block_lock(gl_bheap_state.uniform_block);

    if (gl_bheap_state.uniform_block->uniform_block.sections[ALLOCATION_SIZE_TO_INDEX(allocation_size)] != NULL) {
        PUNIFORM_SECTION curr_head = gl_bheap_state.uniform_block->uniform_block.sections[ALLOCATION_SIZE_TO_INDEX(allocation_size)];
        section->flink = curr_head;
    }
    gl_bheap_state.uniform_block->uniform_block.sections[ALLOCATION_SIZE_TO_INDEX(allocation_size)] = section;

    release_block_lock(gl_bheap_state.uniform_block);
}