/**
 * Ben Williams
 * March 22nd, 2025
 */

#include <../globals.h>
#include <../Headers/macros.h>
#include <../Headers/parameters.h>
#include <block_operations.h>

/**
 * Heap-wide globals to initialize
 */
ULONG_PTR gl_bheap_base;
MEM_EXTENDED_PARAMETER gl_block_mem_parameters;
BHEAP_STATE gl_bheap_state;


/**
 * File-wide globals
 */
MEM_ADDRESS_REQUIREMENTS address_requirements;



/**
 * Initializes the heap.
 * 
 * Reserves memory for the initial dynamic block. Commits DEFAULT_INIT_SIZE memory.
 */
ULONG_PTR init_bheap() {

    gl_block_mem_parameters.Type = MemExtendedParameterAddressRequirements;

    address_requirements.Alignment = BLOCK_SIZE;

    address_requirements.LowestStartingAddress = NULL;
    address_requirements.HighestEndingAddress = NULL;

    gl_block_mem_parameters.Pointer = &address_requirements;

    PBHEAP_BLOCK initial_block = create_new_block(DYNAMIC_BLOCK);

    if (initial_block == NULL) {
        printf("Failed to initialize initial block of heap\n");
        return ERROR;
    }

    address_requirements.LowestStartingAddress = (void*) initial_block;
    gl_bheap_base = (void*) initial_block;

    gl_bheap_state.dynamic_blocks = initial_block; 

    return SUCCESS;
}