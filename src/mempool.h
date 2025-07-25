#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declaration
struct AppState;

// Memory pool size classes (powers of 2 for efficient allocation)
typedef enum {
    POOL_SIZE_16 = 0,    // 16 bytes
    POOL_SIZE_32,        // 32 bytes  
    POOL_SIZE_64,        // 64 bytes
    POOL_SIZE_128,       // 128 bytes
    POOL_SIZE_256,       // 256 bytes
    POOL_SIZE_512,       // 512 bytes
    POOL_SIZE_1024,      // 1024 bytes
    POOL_SIZE_2048,      // 2048 bytes
    POOL_SIZE_COUNT      // Number of size classes
} PoolSizeClass;

// Memory block header for tracking
typedef struct PoolBlock {
    struct PoolBlock *next;  // Next free block in the list
    PoolSizeClass size_class; // Which size class this block belongs to
    uint32_t magic;          // Magic number for corruption detection
} PoolBlock;

// Memory chunk (large allocation that gets subdivided)
typedef struct PoolChunk {
    struct PoolChunk *next;  // Next chunk in the list
    PoolSizeClass size_class; // Size class this chunk serves
    uint8_t *memory;         // Start of memory region
    size_t size;             // Total size of this chunk
    uint32_t block_count;    // Number of blocks in this chunk
    uint32_t used_blocks;    // Number of currently used blocks
} PoolChunk;

// Per-size-class pool information
typedef struct {
    PoolBlock *free_list;    // Head of free block list
    PoolChunk *chunks;       // List of memory chunks for this size class
    uint32_t block_size;     // Size of each block (including header)
    uint32_t blocks_per_chunk; // How many blocks to allocate per chunk
    uint32_t total_blocks;   // Total blocks allocated
    uint32_t used_blocks;    // Currently used blocks
    uint32_t peak_used;      // Peak usage
    uint32_t chunk_count;    // Number of chunks allocated
} PoolSizeInfo;

// Global memory pool state
typedef struct {
    PoolSizeInfo pools[POOL_SIZE_COUNT];
    bool initialized;
    
    // Statistics
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t bytes_allocated;
    uint64_t bytes_deallocated;
    uint64_t peak_memory_usage;
    uint32_t fallback_allocations; // When we fall back to malloc
    
    // Configuration
    uint32_t initial_chunks_per_pool;
    uint32_t max_chunks_per_pool;
    bool enable_corruption_detection;
    bool enable_statistics;
} MemoryPool;

// Magic numbers for corruption detection
#define POOL_BLOCK_MAGIC 0xDEADBEEF
#define POOL_FREE_MAGIC  0xFEEDFACE

// Memory pool functions
bool mempool_init(struct AppState *app_state);
void mempool_cleanup(struct AppState *app_state);
bool mempool_is_initialized(struct AppState *app_state);

// Core allocation functions
void* pool_malloc(size_t size, struct AppState *app_state);
void pool_free(void *ptr, struct AppState *app_state);
void* pool_calloc(size_t count, size_t size, struct AppState *app_state);
void* pool_realloc(void *ptr, size_t new_size, struct AppState *app_state);

// Alternative interface that auto-selects best size class
void* pool_malloc_auto(size_t size, struct AppState *app_state);

// Utility functions
PoolSizeClass mempool_get_size_class(size_t size);
size_t mempool_get_class_size(PoolSizeClass class);
bool mempool_expand_pool(PoolSizeClass class, struct AppState *app_state);

// Debugging and statistics
void mempool_print_stats(struct AppState *app_state);
void mempool_print_detailed_stats(struct AppState *app_state);
bool mempool_validate_integrity(struct AppState *app_state);
size_t mempool_get_total_memory_usage(struct AppState *app_state);
size_t mempool_get_free_memory(struct AppState *app_state);
uint64_t mempool_get_total_usage(struct AppState *app_state);
uint64_t mempool_get_peak_usage(struct AppState *app_state);
bool mempool_validate_all_pools(struct AppState *app_state);

// Configuration (call before mempool_init)
void mempool_set_chunk_limits(struct AppState *app_state, uint32_t initial_chunks, uint32_t max_chunks);
void mempool_set_corruption_detection(struct AppState *app_state, bool enable);
void mempool_set_statistics(struct AppState *app_state, bool enable);

// Optional: Convenience macros for drop-in replacement
#define POOL_MALLOC(size) pool_malloc(size)
#define POOL_FREE(ptr) pool_free(ptr)
#define POOL_CALLOC(count, size) pool_calloc(count, size)
#define POOL_REALLOC(ptr, size) pool_realloc(ptr, size)

#endif 
