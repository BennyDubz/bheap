/**
 * Ben Williams
 * March 22nd, 2025
 */

#include "constants.h"

#define DEBUG_MODE 1

#define DEFAULT_INIT_SIZE (2 * PAGESIZE)

#define BLOCK_SIZE (256 * PAGESIZE)

#define MIN_ALLOCATION_SIZE (16)

#define MAX_SMALL_ALLOCATION (PAGESIZE / 4)

#define NUM_ALLOCATION_SIZES ((MAX_SMALL_ALLOCATION / 8) - 1)


