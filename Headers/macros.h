/**
 * Ben Williams
 * 
 * March 22nd, 2025
 */


/**
 * FUNCTION MACROS
 */
#define ERROR 0
#define SUCCESS 1


/**
 * Conversion macros
 */
#define KB(x) (((ULONG64) x) * 1024)

#define MB(x) (((ULONG64) x) * KB(1024))

#define GB(x) (((ULONG64) x) * MB(1024))

#define PAGESIZE KB(4)