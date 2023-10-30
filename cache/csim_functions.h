/*
 * Cache simulator functions
 * CSF Assignment 3
 * S. Rest and C. Alfonso
 * srest1@jh.edu and calfons5@jh.edu
 */

#ifndef __CSIM_FUNCTIONS_H__
#define __CSIM_FUNCTIONS_H__
#include <vector>
#include <map>
#include <utility>
#include <string.h>

using namespace std;

struct Block {
    uint32_t tag;
    bool valid = false;
    bool dirty = false;
    uint32_t load_ts;
    uint32_t access_ts;
}; 

struct Set {
    std::vector<Block> blocks; // vector of blocks
    std::map<uint32_t, uint32_t> indices; // map of tag to index of slot
};

class CacheSimulator {
public:
    // arguments
    int n_sets;
    int n_blocks;
    int block_size;
    bool is_write_allocate;
    bool is_write_through;
    int is_lru;

    // content
    std::vector<Set> cache; // vector of all sets of blocks in the cache
    std::vector< std::pair<int, uint32_t> > file_data; // vector of pairs of (load/store instruction, address)
    
    // statistics
    uint64_t total_loads = 0;
    uint64_t total_stores = 0;
    uint64_t total_load_hits = 0;
    uint64_t total_load_misses = 0;
    uint64_t total_store_hits = 0;
    uint64_t total_store_misses = 0;
    uint64_t total_cycles = 0;

    /*
     * Constructs a CacheSimulator object without the is_lru parameter.
     *
     * Parameters:
     *  n_sets - number of sets in cache
     *  n_blocks - number of blocks per set in cache
     *  block_size - size of each block in bytes?
     *  is_write_allocate - is the cache write-allocate or no-write-allocate?
     *  is_write_through - is the cache write-through or write-back?
     * 
     * Returns: 
     *  a new CacheSimulator object
     */
    CacheSimulator(const int n_sets, 
                   const int n_blocks, 
                   const int block_size,  
                   const bool is_write_allocate, 
                   const bool is_write_through,
                   const std::vector< std::pair<int, uint32_t> > file_data);

    /*
     * Constructs a CacheSimulator object with the is_lru parameter.
     *
     * Parameters:
     *  n_sets - number of sets in cache
     *  n_blocks - number of blocks per set in cache
     *  block_size - size of each block in bytes
     *  is_write_allocate - is the cache write-allocate or no-write-allocate?
     *  is_write_through - is the cache write-through or write-back?
     *  is_lru - does the cache use lru (least-recently-used) or fifo (first-in-first-out) evictions?
     * 
     * Returns: 
     *  a new CacheSimulator object
     */
    CacheSimulator(const int n_sets, 
                   const int n_blocks, 
                   const int block_size, 
                   const bool is_write_allocate, 
                   const bool is_write_through, 
                   const int is_lru,
                   const std::vector< std::pair<int, uint32_t> > file_data );
    
    /*
     * Prints statistics. 
     */
    void print_counts();

    /*
     * Gets index from address.
     *
     * Parameters:
     *  address - address to get tag from
     * 
     * Returns:
     *  index
     */
    uint32_t get_index(uint32_t address);

    /*
     * Gets tag from address.
     *
     * Parameters:
     *  address - address to get tag from
     * 
     * Returns:
     *  tag
     */
    uint32_t get_tag(uint32_t address);

    /*
     * Returns true if instruction is cache hit, false if cache miss.
     *
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     * 
     * Returns:
     *  index of block within set if cache hit
     *  -1 if cache miss
     */
    int32_t is_hit(uint32_t index, uint32_t tag);

    /*
     * Update access timestamp(s) if using lru evictions
     *
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     *  prev_access_ts - access timestamp of previously evicted, replaced, or added block
     */
    void update_access_ts(uint32_t index, uint32_t tag, uint32_t prev_access_ts);

    /*
     * Evict a block from a set within a cache using lru evictions.
     *
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     */
    void evict_by_lru(uint32_t index, uint32_t tag);

    /*
     * Evict a block from a set within a cache using fifo evictions.
     *
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     */
    void evict_by_fifo(uint32_t index, uint32_t tag);

    /*
     * Add a block to the cache during a cache miss.
     *
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     */
    void add_block(uint32_t index, uint32_t tag);

    /*
     * Update a block at a certain slot within a certain set 
     * during a cache hit.
     * 
     * Parameters:
     *  index - index of cache
     *  tag - target tag of block
     *  block_index - index of block within set
     */
    void update_cache_replica(uint32_t index, uint32_t tag, uint32_t block_index);

    /* 
     * Load an address.
     * 
     * Parameters:
     *  address - the address in main memory to load
     */
    void load(uint32_t address);

    /* 
     * Store an address.
     * 
     * Parameters:
     *  address - the address in main memory to store
     */
    void store(uint32_t address);
    
    /*
     * Runs the cache simulation. 
     */
    void run_simulation();

    /*
     * Return log2 of an integer.
     *
     * Parameters:
     *  num - the integer to get log2 of
     * 
     * Returns:
     *  log2 of num
     */
    int get_log2(int num);
};

#endif