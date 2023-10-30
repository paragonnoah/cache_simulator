#include <vector>
#include <list>
#include <string>

struct Block {
    int tag;
    int index;
};

struct Set {
    std::vector<Block> blocks;
    std::list<int> lruList;
};


class Cache {
public:
    // Constructor to initialize the cache with parameters
    Cache(int numSets, int numBlocks, int numBytes); 

    // Function to process a memory access
    // void access(char accessType, int memoryAddress);

    // Function to display cache statistics
    void displayStatistics();

private:
    // Define cache parameters
    int numSets;         // number of sets in cache
    int numBlocks;         // number of blocks in set
    int numbytes;     // num of bytes in blocks

    // Define cache data structures (sets, blocks, tags, LRU information, etc.)
    // Define variables to track cache statistics (hits, misses, etc.)
    // Implement cache-specific methods for simulation

};