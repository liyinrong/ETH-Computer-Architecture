#include <stdint.h>
#include <stdbool.h>

#define CACHE_BLOCK_SIZE        32
#define ICACHE_SIZE             8192
#define ICACHE_ASSOCIATIVITY    4
// #define ICACHE_SET_NUM          (ICACHE_SIZE / CACHE_BLOCK_SIZE / ICACHE_ASSOCIATIVITY)
#define DCACHE_SIZE             65536
#define DCACHE_ASSOCIATIVITY    8
// #define DCACHE_SET_NUM          (DCACHE_SIZE / CACHE_BLOCK_SIZE / DCACHE_ASSOCIATIVITY)
#define MEMORY_ACCESS_CYCLE     50

typedef struct Cache Cache;
typedef struct Cache_Block Cache_Block;
typedef Cache_Block* (*Cache_Replace_Func)(Cache* cache, uint32_t set_idx);

struct Cache_Block {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint32_t access_timestamp;
    uint8_t data[CACHE_BLOCK_SIZE];
};

struct Cache{
    uint32_t size;
    uint32_t block_size;
    uint32_t associativity;
    uint32_t set_num;
    Cache_Block* blocks;
    Cache_Replace_Func replace;
};

typedef enum Cache_Status {
    CACHE_HIT,
    CACHE_MISS,
    CACHE_BUSY
} Cache_Status;

typedef enum Cache_Replacement_Policy {
    CACHE_REPLACE_LRU
} Cache_Replacement_Policy;

void cache_init(Cache* cache, uint32_t size, uint32_t block_size, uint32_t associativity, Cache_Replacement_Policy policy);
void cache_free(Cache* cache);
Cache_Status cache_access(Cache* cache, uint32_t address, uint32_t* data, bool is_write);
