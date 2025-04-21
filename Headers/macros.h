/**
 * Ben Williams
 * 
 * March 22nd, 2025
 */


#include "parameters.h"
#include "constants.h"

/**
 * Function macros
 */
#define ERROR 0
#define SUCCESS 1

#define ICE_ADD(PU64, ORIG, U64) ((InterlockedCompareExchange64(PU64, ORIG, U64) == ORIG) ? ORIG : ORIG + U64)

/**
 * Conversion macros
 */
#define KB(x) (((ULONG64) x) * 1024)

#define MB(x) (((ULONG64) x) * KB(1024))

#define GB(x) (((ULONG64) x) * MB(1024))


/**
 * Address manipulations
 */

#define DOWN_TO_PAGE(x) (x & ~(PAGE_SIZE - 1))

#define DOWN_TO_BLOCK(x) (x & ~(BLOCK_SIZE - 1))

// Checks for 8-byte alignment
#define SIZE_NOT_ALIGNED(x) (x & 7)

// Rounds up to 8-byte alignment
#define ROUND_UP_SIZE(x) (((x & 7) ? ((x | 7) + 1) : x))

#define ROUND_UP_PAGESIZE(x) ((x & 0xFFF) ? ((x | 0xFFF) + 1) : x)

// For bitwise operations
#define SET_LOWEST_NUMBITS(x) ((1 << x) - 1)