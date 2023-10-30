/*
 * Cache simulator functions
 * CSF Assignment 3
 * S. Rest and C. Alfonso
 * srest1@jh.edu and calfons5@jh.edu
 */

#include <iostream>
#include <iomanip>
#include <bitset>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <string.h>
#include <cmath>
#include <iomanip>
#include "csim_functions.h"

using std::cout;
using std::endl;
using std::cerr;
using std::exception;
using namespace std;

/*
 * Constructs a CacheSimulator object without the is_lru parameter.
 *
 * Parameters:
 *  n_sets - number of sets in cache
 *  n_blocks - number of blocks per set in cache
 *  block_size - size of each block in bytes
 *  is_write_allocate - is the cache write-allocate or no-write-allocate
 *  is_write_through - is the cache write-through or write-back
 * 
 * Returns: 
 *  a new CacheSimulator object
 */
CacheSimulator::CacheSimulator(const int n_sets, 
                               const int n_blocks, 
                               const int block_size,  
                               const bool is_write_allocate, 
                               const bool is_write_through,
                               const std::vector< std::pair<int, uint32_t> > file_data) {
    this->n_sets = n_sets;
    this->n_blocks = n_blocks;
    this->block_size = block_size;
    this->is_write_allocate = is_write_allocate;
    this->is_write_through = is_write_through;
    this->is_lru = -1;
    this->file_data = file_data;

    this->cache.resize(n_sets); // set number of sets to n_sets
}

/*
 * Constructs a CacheSimulator object with the is_lru parameter.
 *
 * Parameters:
 *  n_sets - number of sets in cache
 *  n_blocks - number of blocks per set in cache
 *  block_size - size of each block in bytes
 *  is_write_allocate - is the cache write-allocate or no-write-allocate
 *  is_write_through - is the cache write-through or write-back
 *  is_lru - does the cache use lru (least-recently-used) or fifo (first-in-first-out) evictions
 * 
 * Returns: 
 *  a new CacheSimulator object
 */
CacheSimulator::CacheSimulator(const int n_sets, 
                               const int n_blocks, 
                               const int block_size, 
                               const bool is_write_allocate, 
                               const bool is_write_through, 
                               const int is_lru,
                               const std::vector< std::pair<int, uint32_t> > file_data) {
    this->n_sets = n_sets;
    this->n_blocks = n_blocks;
    this->block_size = block_size;
    this->is_write_allocate = is_write_allocate;
    this->is_write_through = is_write_through;
    this->is_lru = is_lru;
    this->file_data = file_data;

    this->cache.resize(n_sets); // set number of sets to n_sets
}

/*
 * Prints statistics. 
 */
void CacheSimulator::print_counts() {     
    cout << "Total loads: " << total_loads << endl;
    cout << "Total stores: " << total_stores << endl;
    cout << "Load hits: " << total_load_hits << endl;
    cout << "Load misses: " << total_load_misses << endl;
    cout << "Store hits: " << total_store_hits << endl;
    cout << "Store misses: " << total_store_misses << endl;
    cout << "Total cycles: " << total_cycles << endl;
}

/*
 * Gets index from address.
 *
 * Parameters:
 *  address - address to get tag from
 * 
 * Returns:
 *  index
 */
uint32_t CacheSimulator::get_index(uint32_t address) {
    // make index 0 for fully associative caches
    if (n_sets == 1) {
        return 0;
    }
    
    // else get index from num sets
    int index_bits = get_log2(n_sets);
    int32_t mask = (1 << index_bits) - 1;
    return (address >> get_log2(block_size)) & mask;
}

/*
 * Gets tag from address.
 *
 * Parameters:
 *  address - address to get tag from
 * 
 * Returns:
 *  tag
 */
uint32_t CacheSimulator::get_tag(uint32_t address) {
    // for fully associative caches, index + tag combine to become the tag
    if (n_sets == 1) {
        return address >> get_log2(block_size);
    }

    return address >> (get_log2(block_size) + get_log2(n_sets));
}

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
int32_t CacheSimulator::is_hit(uint32_t index, uint32_t tag) {
    Set &s = cache[index];
    auto i = s.indices.find(tag);
    if (i != s.indices.end()) { // hit
        uint32_t index_of_slot = i->second;
        return index_of_slot;
    } else { // miss
        return -1;
    }
}

/*
 * Update access timestamp(s) if using lru evictions.
 *
 * Parameters:
 *  index - index of cache
 *  tag - target tag of block
 *  prev_access_ts - access timestamp of previously evicted, replaced, or added block
 */
void CacheSimulator::update_access_ts(uint32_t index, uint32_t tag, uint32_t prev_access_ts) {
    Set & target_set = cache[index];
    size_t set_length = cache[index].blocks.size();
    for (uint32_t i = 0; i < set_length; i++) {
        Block & block = target_set.blocks[i];
        if (block.access_ts < prev_access_ts && block.tag != tag) {
            block.access_ts++;
        }
    }
}

/*
 * Evict a block from a set within a cache using lru evictions.
 *
 * Parameters:
 *  index - index of cache
 *  tag - target tag of block
 */
void CacheSimulator::evict_by_lru(uint32_t index, uint32_t tag) {
    // find the block that is least recently used in the set (has the highest timestamp value)
    // then use that slot for a new block 
    Set & target_set = cache[index];
    size_t set_length = target_set.blocks.size();
    uint32_t lru_ts = 0;
    uint32_t block_index = 0;
    for (uint32_t i = 0; i < set_length; i++) {
        Block & block = target_set.blocks[i];
        if (block.access_ts > lru_ts) {
            lru_ts = block.access_ts;
            block_index = i;
        }
    }

    Block & block = target_set.blocks[block_index];
    
    // if write-back and block to be evicted is dirty, write dirty block to memory
    if (!is_write_through && block.dirty) {
        total_cycles += 25 * block_size;
    }

    target_set.indices.erase(block.tag); // remove old (tag, block_index) pair from map
    target_set.indices.insert({tag, block_index}); // add (tag, block_index) pair to set map

    // replace slot with new block
    block.tag = tag;
    block.valid = true;
    block.access_ts = 0;
    block.load_ts = total_stores + total_loads;

    // increment all counters
    update_access_ts(index, tag, 0xffffffff);
}

/*
 * Evict a block from a set within a cache using fifo evictions.
 *
 * Parameters:
 *  index - index of cache
 *  tag - target tag of block
 * 
 * Returns:
 *  index of evicted block
 */
void CacheSimulator::evict_by_fifo(uint32_t index, uint32_t tag) {
    // find the block that is least recently loaded in the set (has the lowest timestamp value)
    // then use that slot for a new block 
    Set & target_set = cache[index];
    size_t set_length = target_set.blocks.size();
    uint32_t fifo_ts = 0xffffffff;
    uint32_t block_index = 0;
    for (uint32_t i = 0; i < set_length; i++) {
        Block & block = target_set.blocks[i];
        if (block.load_ts < fifo_ts) {
            fifo_ts = block.load_ts;
            block_index = i;
        }
    }

    Block & block = target_set.blocks[block_index];
    
    // if write-back and block to be evicted is dirty, write dirty block to memory
    if (!is_write_through && block.dirty) {
        total_cycles += 25 * block_size;
    }

    target_set.indices.erase(block.tag); // remove old (tag, block_index) pair from map
    target_set.indices.insert({tag, block_index}); // add (tag, block_index) pair to set map

    // replace slot with new block
    block.tag = tag;
    block.valid = true;
    block.access_ts = 0;
    block.load_ts = total_stores + total_loads;
}

/*
 * Add a block to the cache during a cache miss.
 *
 * Parameters:
 *  index - index of cache
 *  tag - target tag of block
 */
void CacheSimulator::add_block(uint32_t index, uint32_t tag) {
    Set & target_set = cache[index];
    size_t set_size = target_set.blocks.size();

    if (n_blocks > (int) set_size) { // space left in set?
        // add (tag, block_index) pair to set map
        target_set.indices.insert({tag, set_size});
        
        // create new block
        Block * block = new Block();
        block->tag = tag;
        block->valid = true;
        block->access_ts = 0;
        block->load_ts = total_loads + total_stores;
        if (!is_write_through) { // if write-back, mark block as dirty
            block->dirty = true; 
        }
        
        // add to set vector
        target_set.blocks.push_back(*block);

        if (is_lru == 1) { // lru
            update_access_ts(index, tag, 0xffffffff);
        } 
    } else if (n_blocks == 1) { // no space left in direct-mapped cache
        Block & block = target_set.blocks[0];

        target_set.indices.erase(block.tag); // remove old (tag, block_index) pair from set map
        target_set.indices.insert({tag, 0}); // add (tag, block_index) pair to set map

        // replace slot with new block
        block.tag = tag;
        block.valid = true;
        block.access_ts = 0;
        block.load_ts = total_loads + total_stores;
    } else { // no space left in associative cache -> lru or fifo evictions
        if (is_lru == 1) { // lru
            evict_by_lru(index, tag);
        } else { // fifo
            evict_by_fifo(index, tag);
        }
    }
}

/*
 * Update a block at a certain slot within a certain set 
 * during a cache hit.
 * 
 * Parameters:
 *  index - index of cache
 *  tag - target tag of block
 *  block_index - index of block within set
 */
void CacheSimulator::update_cache_replica(uint32_t index, uint32_t tag, uint32_t block_index) {
    Set & target_set = cache[index];
    Block & block = target_set.blocks[block_index];

    // if write-back and block to be evicted is dirty, write dirty block to memory
    if (!is_write_through && block.dirty) {
        total_cycles += 25 * block_size;
    }

    target_set.indices.erase(block.tag); // remove old (tag, block_index) pair from map
    target_set.indices.insert({tag, block_index}); // add (tag, block_index) pair to set map
    
    block.tag = tag; // replace tag
    // set access_ts to 0
    uint32_t prev_access_ts = block.access_ts;
    block.access_ts = 0;
    // if write-back, mark block as dirty
    if (!is_write_through) {
        block.dirty = true;
    }

    // update access timestamps of blocks with timestamp < replaced block's timestamp
    if (is_lru == 1) {
        update_access_ts(index, tag, prev_access_ts);
    }
}

/* 
 * Load an address.
 * 
 * Parameters:
 *  address - the address in main memory to load
 */
void CacheSimulator::load(uint32_t address) {
    uint32_t index = get_index(address);
    uint32_t tag = get_tag(address);

    int32_t block_index = is_hit(index, tag); 
    if (block_index >= 0) { // cache hit
        if (is_lru == 1) {
            Block & block = cache[index].blocks[block_index];
            uint32_t prev_access_ts = block.access_ts; // save access_ts of block
            block.access_ts = 0; // reset access_ts
            update_access_ts(index, tag, prev_access_ts); // update counters with access ts < prev_access_ts
        }
        total_load_hits++;
    } else { // cache miss
        add_block(index, tag);
        total_load_misses++;
        total_cycles += 25 * block_size; // load from memory
    }

    total_cycles++; // access data in cache
    total_loads++;
}

/* 
 * Store an address.
 * 
 * Parameters:
 *  address - the address in main memory to store
 */
void CacheSimulator::store(uint32_t address) {
    uint32_t index = get_index(address);
    uint32_t tag = get_tag(address);
    
    int32_t block_index = is_hit(index, tag);
    if (block_index >= 0) { // cache hit
        total_store_hits++;
        if (is_write_through) {
            total_cycles += 100; // store new value in memory
        }
        update_cache_replica(index, tag, block_index);
        total_cycles++; // store in cache
    } else { // cache miss
        if (is_write_allocate) { // retrieve from memory and load into cache
            add_block(index, tag);
            total_cycles += 25 * block_size; // retrieve from memory
            total_cycles++;
        } else { // no-write-allocate
            total_cycles += 100; // store new value in memory
        }
        total_store_misses++;
    }
    total_stores++;
}

/*
 * Runs the cache simulation. 
 */
void CacheSimulator::run_simulation() {
    for (unsigned long i = 0; i < file_data.size(); i++) {
        if (file_data[i].first == 1) { // operation: store
            store(file_data[i].second);
        } else { // operation: load
            load(file_data[i].second);
        }
    }
    print_counts();
}

/*
 * Return log2 of an integer.
 *
 * Parameters:
 *  num - the integer to get log2 of
 * 
 * Returns:
 *  log2 of num
 */
int CacheSimulator::get_log2(int num) {
    float x = log2(num);
    x = x + 0.5 - (x < 0);
    return (int) x;
}

/*
 * Prints invalid arguments to cerr and returns 1.
 *
 * Returns: 1
 */
int invalid_args() {
    cerr << "Invalid arguments" << endl;
    return 1;
}

/*
 * Load valid arguments and run cache simulation.
 * 
 * Returns:
 *  0 if cache simulation successful
 *  1 if cache simulation unsuccessful
 */
int main(int argc, char * argv[]) {
    // validate arguments
    if (argc < 6 || argc > 7) {
       return(invalid_args());
    } else {
        vector< pair<int, uint32_t> > file_data;
        // read in memory trace data
        string trace_data;
        while (getline(cin, trace_data)) {
            stringstream ss(trace_data);
            string fields[3], field; // fields[0]: s or l, fields[1]: memory address (0xhexadecimal), fields[2]: integer (ignore)
            int counter = 0;
            while (ss >> field) {
                fields[counter] = field;
                counter++;
            }

            // store data from each line in a vector
            // pair with fields[0] and fields[1] stored in vector; fields[2] is ignored for this assignment
            string address = fields[1].erase(0, 2); // removes 0x from the beginning of address
            bool is_store = fields[0] == "s";
            file_data.push_back(make_pair(is_store, std::stoul(address, nullptr, 16)));
        }

        int n_sets, n_blocks, block_size;
        try {
            n_sets = atoi(argv[1]);
            n_blocks = atoi(argv[2]);
            block_size = atoi(argv[3]);
        } catch (std::exception & e) {
            return(invalid_args());
        }

        // if n_sets is 0, negative, or not power of 2, error
        // if n_blocks is 0, negative, or not power of 2, error
        // if block_size is less than 4 or not power of 2, error
        if (n_sets <= 0 || (n_sets & (n_sets - 1)) != 0 
            || n_blocks <= 0 || (n_blocks & (n_blocks - 1)) != 0 
            || block_size < 4 || (block_size & (block_size - 1)) != 0) {
            return(invalid_args());
        }

        bool is_write_allocate;
        // argv[3] must be "write-allocate" or "no-write-allocate"
        if (strcmp(argv[4], "write-allocate" ) == 0 ) {
            is_write_allocate = true;
        } else if ( strcmp(argv[4], "no-write-allocate" ) == 0) {
            is_write_allocate = false;
        } else {
            return(invalid_args());
        }

        bool is_write_through;
        // argv[4] must be "write-through" or "write-back"
        if (strcmp(argv[5], "write-through" ) == 0) {
            // is_write_through is true
            is_write_through = true;
        } else if (strcmp(argv[5], "write-back" ) == 0) {
            // is_write_through is false
            is_write_through = false;
        } else {
            return(invalid_args());
        }
        // no-write-allocate cannot be combined with with write-back
        if (!is_write_allocate && !is_write_through) {
            return(invalid_args());
        }

        // check if lru/fifo arg provided
        if (argc > 6) {
            int is_lru;
            if (strcmp(argv[6], "lru") == 0 ) {
                is_lru = 1;
            } else if (strcmp(argv[6], "fifo" ) == 0) {
                is_lru = 0;
            } else {
                return(invalid_args());
            }
            // construct CacheSimulator with is_lru arg
            CacheSimulator * cache = new CacheSimulator(n_sets, n_blocks, block_size, is_write_allocate, is_write_through, is_lru, file_data);
            cache->run_simulation();
        } else {
            // if no lru/fifo arg provided, cache must be direct-mapped
            if (n_blocks != 1) {
                return(invalid_args());
            }
            // construct CacheSimulator without is_lru arg
            CacheSimulator * cache = new CacheSimulator(n_sets, n_blocks, block_size, is_write_allocate, is_write_through, file_data);
            cache->run_simulation();
        }
    }

	return 0;
}