#include "mempool.h"
#include "log.h"
#include "error.h"
#include "appstate.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Size class configurations (size in bytes, blocks per chunk)
static const struct {
    uint32_t size;
    uint32_t blocks_per_chunk;
} SIZE_CLASS_CONFIG[POOL_SIZE_COUNT] = {
    {16,   512},  // POOL_SIZE_16:   8KB chunks (more for frequent small allocs)
    {32,   256},  // POOL_SIZE_32:   8KB chunks (more for frequent small allocs)  
    {64,   256},  // POOL_SIZE_64:   16KB chunks (much more for linked list nodes)
    {128,  128},  // POOL_SIZE_128:  16KB chunks (more for medium allocs)
    {256,  64},   // POOL_SIZE_256:  16KB chunks (more reasonable)
    {512,  32},   // POOL_SIZE_512:  16KB chunks
    {1024, 16},   // POOL_SIZE_1024: 16KB chunks
    {2048, 8},    // POOL_SIZE_2048: 16KB chunks
};

// Forward declarations
static bool allocate_new_chunk(PoolSizeClass class, struct AppState *app_state);
static PoolBlock* get_block_from_pool(PoolSizeClass class, struct AppState *app_state);
static void return_block_to_pool(PoolBlock *block, struct AppState *app_state);
static bool is_valid_pool_pointer(void *ptr, struct AppState *app_state);
static void update_statistics(PoolSizeClass class, bool allocating, struct AppState *app_state);

// Get the appropriate size class for a given size
PoolSizeClass mempool_get_size_class(size_t size) {
    // Add space for the block header
    size += sizeof(PoolBlock);
    
    if (size <= 16) return POOL_SIZE_16;
    if (size <= 32) return POOL_SIZE_32;
    if (size <= 64) return POOL_SIZE_64;
    if (size <= 128) return POOL_SIZE_128;
    if (size <= 256) return POOL_SIZE_256;
    if (size <= 512) return POOL_SIZE_512;
    if (size <= 1024) return POOL_SIZE_1024;
    if (size <= 2048) return POOL_SIZE_2048;
    
    return POOL_SIZE_COUNT; // Too large for pool allocation
}

// Get the actual size for a size class
size_t mempool_get_class_size(PoolSizeClass class) {
    if (class >= POOL_SIZE_COUNT) return 0;
    return SIZE_CLASS_CONFIG[class].size;
}

// Initialize the memory pool system
bool mempool_init(struct AppState *app_state) {
    if (app_state->mempool.initialized) {
        LOG_WARN("Memory pool already initialized");
        return true;
    }
    
    memset(&app_state->mempool, 0, sizeof(MemoryPool));
    
    // Set default configuration
    app_state->mempool.initial_chunks_per_pool = 1;
    app_state->mempool.max_chunks_per_pool = 64;
    app_state->mempool.enable_corruption_detection = true;
    app_state->mempool.enable_statistics = true;
    
    // Initialize each pool size class
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        pool->block_size = SIZE_CLASS_CONFIG[i].size;
        pool->blocks_per_chunk = SIZE_CLASS_CONFIG[i].blocks_per_chunk;
        pool->free_list = NULL;
        pool->chunks = NULL;
        pool->total_blocks = 0;
        pool->used_blocks = 0;
        pool->peak_used = 0;
        pool->chunk_count = 0;
        
        // Allocate initial chunk for each size class
        if (!allocate_new_chunk((PoolSizeClass)i, app_state)) {
            LOG_ERROR("Failed to allocate initial chunk for size class %d", i);
            mempool_cleanup(app_state);
            return false;
        }
    }
    
    app_state->mempool.initialized = true;
    LOG_INFO("Memory pool initialized with %d size classes", POOL_SIZE_COUNT);
    return true;
}

// Cleanup the memory pool system
void mempool_cleanup(struct AppState *app_state) {
    if (!app_state->mempool.initialized) {
        return;
    }
    
    // Print final statistics
    if (app_state->mempool.enable_statistics) {
        mempool_print_stats(app_state);
    }
    
    // Free all chunks
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        PoolChunk *chunk = pool->chunks;
        
        while (chunk) {
            PoolChunk *next = chunk->next;
            free(chunk->memory);
            free(chunk);
            chunk = next;
        }
    }
    
    memset(&app_state->mempool, 0, sizeof(MemoryPool));
    LOG_INFO("Memory pool cleaned up");
}

// Check if memory pool is initialized
bool mempool_is_initialized(struct AppState *app_state) {
    return app_state->mempool.initialized;
}

// Allocate a new chunk for a specific size class
static bool allocate_new_chunk(PoolSizeClass class, struct AppState *app_state) {
    if (class >= POOL_SIZE_COUNT) return false;
    
    PoolSizeInfo *pool = &app_state->mempool.pools[class];
    
    // Check if we've hit the maximum chunk limit
    if (pool->chunk_count >= app_state->mempool.max_chunks_per_pool) {
        LOG_DEBUG("Hit maximum chunk limit for size class %d (%u chunks), allowing fallback", 
                  class, pool->chunk_count);
        return false;
    }
    
    // Calculate chunk size
    uint32_t block_size = pool->block_size;
    uint32_t block_count = pool->blocks_per_chunk;
    size_t chunk_size = block_size * block_count;
    
    // Allocate memory for the chunk descriptor
    PoolChunk *chunk = malloc(sizeof(PoolChunk));
    if (!chunk) {
        LOG_ERROR("Failed to allocate chunk descriptor");
        return false;
    }
    
    // Allocate the actual memory
    chunk->memory = malloc(chunk_size);
    if (!chunk->memory) {
        LOG_ERROR("Failed to allocate chunk memory (%zu bytes)", chunk_size);
        free(chunk);
        return false;
    }
    
    // Initialize chunk
    chunk->next = pool->chunks;
    chunk->size_class = class;
    chunk->size = chunk_size;
    chunk->block_count = block_count;
    chunk->used_blocks = 0;
    
    // Add chunk to the pool's chunk list
    pool->chunks = chunk;
    pool->chunk_count++;
    pool->total_blocks += block_count;
    
    // Initialize free list for this chunk
    uint8_t *memory = chunk->memory;
    for (uint32_t i = 0; i < block_count; i++) {
        PoolBlock *block = (PoolBlock*)(memory + i * block_size);
        block->next = pool->free_list;
        block->size_class = class;
        block->magic = POOL_FREE_MAGIC;
        pool->free_list = block;
    }
    
    LOG_DEBUG("Allocated new chunk for size class %d: %u blocks, %zu bytes (chunk %u/%u)", 
              class, block_count, chunk_size, pool->chunk_count, app_state->mempool.max_chunks_per_pool);
    return true;
}

// Get a block from the specified pool
static PoolBlock* get_block_from_pool(PoolSizeClass class, struct AppState *app_state) {
    if (class >= POOL_SIZE_COUNT) return NULL;
    
    PoolSizeInfo *pool = &app_state->mempool.pools[class];
    
    // If no free blocks, try to allocate a new chunk
    if (!pool->free_list) {
        if (!allocate_new_chunk(class, app_state)) {
            return NULL;
        }
    }
    
    // Get block from free list
    PoolBlock *block = pool->free_list;
    if (!block) return NULL;
    
    pool->free_list = block->next;
    
    // Update statistics
    pool->used_blocks++;
    if (pool->used_blocks > pool->peak_used) {
        pool->peak_used = pool->used_blocks;
    }
    
    // Find which chunk this block belongs to and update its usage
    PoolChunk *chunk = pool->chunks;
    while (chunk) {
        uint8_t *chunk_start = chunk->memory;
        uint8_t *chunk_end = chunk_start + chunk->size;
        
        if ((uint8_t*)block >= chunk_start && (uint8_t*)block < chunk_end) {
            chunk->used_blocks++;
            break;
        }
        chunk = chunk->next;
    }
    
    // Set up block header
    block->next = NULL;
    block->size_class = class;
    if (app_state->mempool.enable_corruption_detection) {
        block->magic = POOL_BLOCK_MAGIC;
    }
    
    return block;
}

// Return a block to its pool
static void return_block_to_pool(PoolBlock *block, struct AppState *app_state) {
    if (!block) return;
    
    PoolSizeClass class = block->size_class;
    if (class >= POOL_SIZE_COUNT) {
        LOG_ERROR("Invalid size class in block: %d", class);
        return;
    }
    
    // Corruption detection
    if (app_state->mempool.enable_corruption_detection && block->magic != POOL_BLOCK_MAGIC) {
        LOG_ERROR("Block corruption detected: expected magic 0x%08X, got 0x%08X", 
                  POOL_BLOCK_MAGIC, block->magic);
        return;
    }
    
    PoolSizeInfo *pool = &app_state->mempool.pools[class];
    
    // Update statistics
    pool->used_blocks--;
    
    // Find which chunk this block belongs to and update its usage
    PoolChunk *chunk = pool->chunks;
    while (chunk) {
        uint8_t *chunk_start = chunk->memory;
        uint8_t *chunk_end = chunk_start + chunk->size;
        
        if ((uint8_t*)block >= chunk_start && (uint8_t*)block < chunk_end) {
            chunk->used_blocks--;
            break;
        }
        chunk = chunk->next;
    }
    
    // Add block back to free list
    block->magic = POOL_FREE_MAGIC;
    block->next = pool->free_list;
    pool->free_list = block;
}

// Check if a pointer is from the pool
static bool is_valid_pool_pointer(void *ptr, struct AppState *app_state) {
    if (!ptr || !app_state->mempool.initialized) return false;
    
    PoolBlock *block = (PoolBlock*)((uint8_t*)ptr - sizeof(PoolBlock));
    
    // Check if this block is within any of our chunks
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolChunk *chunk = app_state->mempool.pools[i].chunks;
        while (chunk) {
            uint8_t *chunk_start = chunk->memory;
            uint8_t *chunk_end = chunk_start + chunk->size;
            
            if ((uint8_t*)block >= chunk_start && (uint8_t*)block < chunk_end) {
                return true;
            }
            chunk = chunk->next;
        }
    }
    
    return false;
}

// Update allocation statistics
static void update_statistics(PoolSizeClass class, bool allocating, struct AppState *app_state) {
    if (!app_state->mempool.enable_statistics) return;
    
    if (allocating) {
        app_state->mempool.total_allocations++;
        app_state->mempool.bytes_allocated += mempool_get_class_size(class);
    } else {
        app_state->mempool.total_deallocations++;
        app_state->mempool.bytes_deallocated += mempool_get_class_size(class);
    }
    
    // Update peak memory usage
    size_t current_usage = mempool_get_total_memory_usage(app_state);
    if (current_usage > app_state->mempool.peak_memory_usage) {
        app_state->mempool.peak_memory_usage = current_usage;
    }
}

// Core allocation function
void* pool_malloc(size_t size, struct AppState *app_state) {
    if (!app_state->mempool.initialized) {
        LOG_DEBUG("Memory pool not initialized, falling back to malloc");
        app_state->mempool.fallback_allocations++;
        return malloc(size);
    }
    
    if (size == 0) return NULL;
    
    PoolSizeClass class = mempool_get_size_class(size);
    
    // If size is too large for pool, fall back to malloc
    if (class >= POOL_SIZE_COUNT) {
        app_state->mempool.fallback_allocations++;
        return malloc(size);
    }
    
    PoolBlock *block = get_block_from_pool(class, app_state);
    if (!block) {
        LOG_DEBUG("Pool exhausted for size class %d, falling back to malloc (fallback #%u)", 
                  class, app_state->mempool.fallback_allocations + 1);
        app_state->mempool.fallback_allocations++;
        return malloc(size);
    }
    
    update_statistics(class, true, app_state);
    
    // Return pointer to user data (after the header)
    return (uint8_t*)block + sizeof(PoolBlock);
}

// Core deallocation function
void pool_free(void *ptr, struct AppState *app_state) {
    if (!ptr) return;
    
    // Check if this pointer is from our pool
    if (!is_valid_pool_pointer(ptr, app_state)) {
        // Not from pool, use regular free
        free(ptr);
        return;
    }
    
    // Get the block header
    PoolBlock *block = (PoolBlock*)((uint8_t*)ptr - sizeof(PoolBlock));
    
    update_statistics(block->size_class, false, app_state);
    return_block_to_pool(block, app_state);
}

// Calloc implementation
void* pool_calloc(size_t count, size_t size, struct AppState *app_state) {
    size_t total_size = count * size;
    void *ptr = pool_malloc(total_size, app_state);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Realloc implementation
void* pool_realloc(void *ptr, size_t new_size, struct AppState *app_state) {
    if (!ptr) return pool_malloc(new_size, app_state);
    if (new_size == 0) {
        pool_free(ptr, app_state);
        return NULL;
    }
    
    // If not from pool, use regular realloc
    if (!is_valid_pool_pointer(ptr, app_state)) {
        return realloc(ptr, new_size);
    }
    
    // Get old size class
    PoolBlock *old_block = (PoolBlock*)((uint8_t*)ptr - sizeof(PoolBlock));
    PoolSizeClass old_class = old_block->size_class;
    PoolSizeClass new_class = mempool_get_size_class(new_size);
    
    // If same size class, no need to reallocate
    if (new_class == old_class) {
        return ptr;
    }
    
    // Allocate new memory
    void *new_ptr = pool_malloc(new_size, app_state);
    if (!new_ptr) return NULL;
    
    // Copy old data
    size_t old_size = mempool_get_class_size(old_class) - sizeof(PoolBlock);
    size_t copy_size = (new_size < old_size) ? new_size : old_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old memory
    pool_free(ptr, app_state);
    
    return new_ptr;
}

// Expand a specific pool
bool mempool_expand_pool(PoolSizeClass class, struct AppState *app_state) {
    if (!app_state->mempool.initialized || class >= POOL_SIZE_COUNT) {
        return false;
    }
    
    return allocate_new_chunk(class, app_state);
}

// Print basic statistics
void mempool_print_stats(struct AppState *app_state) {
    if (!app_state->mempool.initialized) {
        LOG_INFO("Memory pool not initialized");
        return;
    }
    
    LOG_INFO("=== Memory Pool Statistics ===");
    LOG_INFO("Total allocations: %llu", (unsigned long long)app_state->mempool.total_allocations);
    LOG_INFO("Total deallocations: %llu", (unsigned long long)app_state->mempool.total_deallocations);
    LOG_INFO("Bytes allocated: %llu", (unsigned long long)app_state->mempool.bytes_allocated);
    LOG_INFO("Bytes deallocated: %llu", (unsigned long long)app_state->mempool.bytes_deallocated);
    LOG_INFO("Peak memory usage: %llu bytes", (unsigned long long)app_state->mempool.peak_memory_usage);
    LOG_INFO("Fallback allocations: %u", app_state->mempool.fallback_allocations);
    LOG_INFO("Current memory usage: %zu bytes", mempool_get_total_memory_usage(app_state));
}

// Print detailed statistics
void mempool_print_detailed_stats(struct AppState *app_state) {
    mempool_print_stats(app_state);
    
    LOG_INFO("=== Per-Size-Class Statistics ===");
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        LOG_INFO("Size class %d (%u bytes): %u/%u blocks used, %u chunks, peak: %u",
                 i, pool->block_size, pool->used_blocks, pool->total_blocks,
                 pool->chunk_count, pool->peak_used);
    }
}

// Get total memory usage
size_t mempool_get_total_memory_usage(struct AppState *app_state) {
    if (!app_state->mempool.initialized) return 0;
    
    size_t total = 0;
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        total += pool->used_blocks * pool->block_size;
    }
    return total;
}

// Get free memory
size_t mempool_get_free_memory(struct AppState *app_state) {
    if (!app_state->mempool.initialized) return 0;
    
    size_t total = 0;
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        total += (pool->total_blocks - pool->used_blocks) * pool->block_size;
    }
    return total;
}

// Validate pool integrity
bool mempool_validate_integrity(struct AppState *app_state) {
    if (!app_state->mempool.initialized) return false;
    
    bool valid = true;
    
    for (int i = 0; i < POOL_SIZE_COUNT; i++) {
        PoolSizeInfo *pool = &app_state->mempool.pools[i];
        
        // Check that used_blocks doesn't exceed total_blocks
        if (pool->used_blocks > pool->total_blocks) {
            LOG_ERROR("Pool %d: used_blocks (%u) > total_blocks (%u)", 
                      i, pool->used_blocks, pool->total_blocks);
            valid = false;
        }
        
        // Validate free list if corruption detection is enabled
        if (app_state->mempool.enable_corruption_detection) {
            PoolBlock *block = pool->free_list;
            uint32_t free_count = 0;
            
            while (block && free_count < pool->total_blocks) {
                if (block->magic != POOL_FREE_MAGIC) {
                    LOG_ERROR("Pool %d: free block has invalid magic: 0x%08X", 
                              i, block->magic);
                    valid = false;
                    break;
                }
                
                if (block->size_class != (PoolSizeClass)i) {
                    LOG_ERROR("Pool %d: free block has wrong size class: %d", 
                              i, block->size_class);
                    valid = false;
                    break;
                }
                
                block = block->next;
                free_count++;
            }
        }
    }
    
    return valid;
}

// Configuration functions
void mempool_set_chunk_limits(struct AppState *app_state, uint32_t initial_chunks, uint32_t max_chunks) {
    if (app_state->mempool.initialized) {
        LOG_WARN("Cannot change chunk count after initialization");
        return;
    }
    
    app_state->mempool.initial_chunks_per_pool = initial_chunks;
    app_state->mempool.max_chunks_per_pool = max_chunks;
}

void mempool_set_corruption_detection(struct AppState *app_state, bool enable) {
    app_state->mempool.enable_corruption_detection = enable;
}

void mempool_set_statistics(struct AppState *app_state, bool enable) {
    app_state->mempool.enable_statistics = enable;
} 
