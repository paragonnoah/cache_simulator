#include "cache_sim.h"

void cacheSimulation (u_int32_t sets, u_int32_t blocks, u_int32_t bytes, std::string writeAllocate, std::string writeThrough, std::string eviction) {
    this->sets = sets;
    this->blocks = blocks;
    this->bytes = bytes;
    this->writeAllocate = writeAllocate;
    this->writeThrough = writeThrough;
    //evict fifo
    //evict lui
    this->cache.resize(sets);

}

void cacheSimulation::printCount() {
    cout << "Total loads: " << total_loads << endl;
    cout << "Total stores: " << total_stores << endl;
    cout << "Load hits: " << total_load_hits << endl;
    cout << "Load misses: " << total_load_misses << endl;
    cout << "Store hits: " << total_store_hits << endl;
    cout << "Store misses: " << total_store_misses << endl;
    cout << "Total cycles: " << total_cycles << endl;
}


int main (int argc, char * argv[]) {
    if (argc < 6 || argc > 7) {
        cerr << "Invalid argument" << endl;
        return 1;
    }
    else {
        vector<pair<int, uint32_t>> file;
        
    }
}


