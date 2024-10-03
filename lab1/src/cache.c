#include "cache.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

extern uint32_t stat_cycles;

static Cache_Block* _cache_lru_replace(Cache* cache, uint32_t set_idx);

void cache_init(Cache* cache, uint32_t size, uint32_t block_size, uint32_t associativity, Cache_Replacement_Policy policy)
{
    cache->size = size;
    cache->block_size = block_size;
    cache->associativity = associativity;
    cache->set_num = size / block_size / associativity;    
    cache->blocks = (Cache_Block*)calloc(cache->set_num * associativity, sizeof(Cache_Block));
    if (cache->blocks == NULL) {
        printf("Failed to allocate memory for cache blocks.\r\n");
        exit(1);
    }
    switch (policy) {
        case CACHE_REPLACE_LRU:
            cache->replace = _cache_lru_replace;
            break;
        default:
            printf("Unimplemented cache replacement policy.\r\n");
            exit(1);
    }
}

void cache_free(Cache* cache)
{
    free(cache->blocks);
    memset(cache, 0, sizeof(Cache));
}

Cache_Status cache_access(Cache *cache, uint32_t address, uint32_t *data, bool is_write)
{
    static uint32_t delay_cycles = 0;
    static uint32_t readin_address = 0;
    static uint32_t writeback_address = 0;
    static Cache_Block* replace_block = NULL;
    // if(delay_cycles > 0)
    // {
    //     delay_cycles--;
    //     return CACHE_BUSY;
    // }
    // else if(readin_address == 0)
    // {
    //     readin_address = address;
    //     delay_cycles = MEMORY_ACCESS_CYCLE;
    //     return CACHE_MISS;
    // }
    // else
    // {
    //     if(is_write)
    //     {
    //         mem_write_32(address, *data);
    //     }
    //     else
    //     {
    //         *data = mem_read_32(address);
    //     }
    //     readin_address = 0;
    //     return CACHE_HIT;
    // }

    if(delay_cycles > 0) {
        delay_cycles--;
        return CACHE_BUSY;
    }
    else if(replace_block != NULL) {
        if(replace_block->dirty) {
            for(int i = 0; i < cache->block_size / 4; i++) {
                mem_write_32(writeback_address + i * 4, *(uint32_t*)(replace_block->data + i * 4));
            }
            replace_block->dirty = false;
        }
        replace_block->tag = readin_address / cache->block_size / cache->set_num;
        for(int i = 0; i < cache->block_size / 4; i++) {
            *(uint32_t*)(replace_block->data + i * 4) = mem_read_32(readin_address + i * 4);
        }
        replace_block->valid = true;
        replace_block->access_timestamp = stat_cycles;
        replace_block = NULL;
    }
    uint32_t block_idx = address / cache->block_size;
    uint32_t set_idx = block_idx % cache->set_num;
    uint32_t tag = block_idx / cache->set_num;
    Cache_Block* block = cache->blocks + set_idx * cache->associativity;
    // printf("address: %08x, block_idx: %d, set_idx: %d, tag: %d\n", address, block_idx, set_idx, tag);
    for (int i = 0; i < cache->associativity; i++) {
        if ((block + i)->valid && (block + i)->tag == tag) {
            if (is_write) {
                memcpy((block + i)->data + (address % cache->block_size), data, sizeof(uint32_t));
                (block + i)->dirty = true;
            } else {
                memcpy(data, (block + i)->data + (address % cache->block_size), sizeof(uint32_t));
            }
            (block + i)->access_timestamp = stat_cycles;
            return CACHE_HIT;
        }
    }
    replace_block = cache->replace(cache, set_idx);
    readin_address = block_idx * cache->block_size;
    writeback_address = (replace_block->tag * cache->set_num + (replace_block - cache->blocks) / cache->associativity) * cache->block_size;
    delay_cycles = MEMORY_ACCESS_CYCLE;
    return CACHE_MISS;
}

Cache_Block *_cache_lru_replace(Cache *cache, uint32_t set_idx)
{
    Cache_Block* blocks = cache->blocks + set_idx * cache->associativity;
    Cache_Block* lru_block = blocks;
    for (int i = 1; i < cache->associativity; i++) {
        if (blocks[i].access_timestamp < lru_block->access_timestamp) {
            lru_block = blocks + i;
        }
    }
    return lru_block;
}
