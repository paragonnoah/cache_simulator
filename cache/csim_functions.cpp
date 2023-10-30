#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "csim_functions.h"

// CacheBlock represents a cache block
struct CacheBlock {
    uint32_t tag;
    bool valid;
    bool dirty;
    uint32_t accessTime;
    uint32_t loadTime;
};

// CacheSet represents a cache set
struct CacheSet {
    std::vector<CacheBlock> blocks;
    std::unordered_map<uint32_t, size_t> tagToIndex;
};

class CacheSimulatorAlternative {
public:
    int numSets;
    int numBlocks;
    int blockSize;
    bool writeAllocate;
    bool writeThrough;
    int evictionPolicy;
    std::vector<std::pair<int, uint32_t>> memoryTraces;

    uint64_t loadHits;
    uint64_t loadMisses;
    uint64_t storeHits;
    uint64_t storeMisses;
    uint64_t totalCycles;

    std::vector<CacheSet> cache;

    CacheSimulatorAlternative(int nSets, int nBlocks, int blockSz, bool writeAlloc, bool writeThru, int evictionPol,
                          std::vector<std::pair<int, uint32_t>> traces)
        : numSets(nSets), numBlocks(nBlocks), blockSize(blockSz),
          writeAllocate(writeAlloc), writeThrough(writeThru), evictionPolicy(evictionPol),
          memoryTraces(std::move(traces)), loadHits(0), loadMisses(0), storeHits(0),
          storeMisses(0), totalCycles(0) {
        cache.resize(numSets);
    }

    void printCounts() {
        std::cout << "Total loads: " << loadHits + loadMisses << std::endl;
        std::cout << "Total stores: " << storeHits + storeMisses << std::endl;
        std::cout << "Load hits: " << loadHits << std::endl;
        std::cout << "Load misses: " << loadMisses << std::endl;
        std::cout << "Store hits: " << storeHits << std::endl;
        std::cout << "Store misses: " << storeMisses << std::endl;
        std::cout << "Total cycles: " << totalCycles << std::endl;
    }

    uint32_t getIndex(uint32_t address) {
        if (numSets == 1) {
            return 0;
        }
        int indexBits = static_cast<int>(std::log2(numSets));
        int32_t mask = (1 << indexBits) - 1;
        return (address >> static_cast<int>(std::log2(blockSize))) & mask;
    }

    uint32_t getTag(uint32_t address) {
        if (numSets == 1) {
            return address >> static_cast<int>(std::log2(blockSize));
        }
        return address >> (static_cast<int>(std::log2(blockSize)) + static_cast<int>(std::log2(numSets)));
    }

    int32_t isHit(uint32_t index, uint32_t tag) {
        CacheSet& set = cache[index];
        auto it = set.tagToIndex.find(tag);
        if (it != set.tagToIndex.end()) {
            return static_cast<int32_t>(it->second);
        }
        return -1;
    }

    void evict(uint32_t index, uint32_t tag) {
        CacheSet& set = cache[index];
        if (set.blocks.size() < static_cast<size_t>(numBlocks)) {
            // Cache is not full
            CacheBlock block;
            block.tag = tag;
            block.valid = true;
            block.dirty = false;
            block.accessTime = 0;
            block.loadTime = loadHits + loadMisses;
            set.blocks.push_back(block);
            set.tagToIndex[tag] = set.blocks.size() - 1;
            if (evictionPolicy == 1) {
                updateAccessTime(index, tag, 0xffffffff);
            }
        } else if (evictionPolicy == 1) {
            evictLRU(index, tag);
        } else {
            evictFIFO(index, tag);
        }
    }

    void evictLRU(uint32_t index, uint32_t tag) {
        CacheSet& set = cache[index];
        uint32_t lruTime = 0;
        size_t blockIndex = 0;
        for (size_t i = 0; i < set.blocks.size(); i++) {
            if (set.blocks[i].accessTime > lruTime) {
                lruTime = set.blocks[i].accessTime;
                blockIndex = i;
            }
        }
        CacheBlock& block = set.blocks[blockIndex];
        if (!writeThrough && block.dirty) {
            totalCycles += 25 * blockSize;
        }
        set.tagToIndex.erase(block.tag);
        set.tagToIndex[tag] = blockIndex;
        block.tag = tag;
        block.valid = true;
        block.accessTime = 0;
        block.loadTime = loadHits + loadMisses;
        if (!writeThrough) {
            block.dirty = true;
        }
        updateAccessTime(index, tag, 0xffffffff);
    }

    void evictFIFO(uint32_t index, uint32_t tag) {
        CacheSet& set = cache[index];
        uint32_t fifoTime = 0xffffffff;
        size_t blockIndex = 0;
        for (size_t i = 0; i < set.blocks.size(); i++) {
            if (set.blocks[i].loadTime < fifoTime) {
                fifoTime = set.blocks[i].loadTime;
                blockIndex = i;
            }
        }
        CacheBlock& block = set.blocks[blockIndex];
        if (!writeThrough && block.dirty) {
            totalCycles += 25 * blockSize;
        }
        set.tagToIndex.erase(block.tag);
        set.tagToIndex[tag] = blockIndex;
        block.tag = tag;
        block.valid = true;
        block.accessTime = 0;
        block.loadTime = loadHits + loadMisses;
    }

    void updateAccessTime(uint32_t index, uint32_t tag, uint32_t prevAccessTime) {
        CacheSet& set = cache[index];
        for (CacheBlock& block : set.blocks) {
            if (block.accessTime < prevAccessTime && block.tag != tag) {
                block.accessTime++;
            }
        }
    }

    void load(uint32_t address) {
        uint32_t index = getIndex(address);
        uint32_t tag = getTag(address);
        int32_t blockIndex = isHit(index, tag);
        if (blockIndex != -1) {
            loadHits++;
            if (evictionPolicy == 1) {
                updateAccessTime(index, tag, cache[index].blocks[blockIndex].accessTime);
            }
        } else {
            loadMisses++;
            totalCycles += 100;
            evict(index, tag);
        }
    }

    void store(uint32_t address) {
        uint32_t index = getIndex(address);
        uint32_t tag = getTag(address);
        int32_t blockIndex = isHit(index, tag);
        if (blockIndex != -1) {
            storeHits++;
            if (!writeThrough) {
                cache[index].blocks[blockIndex].dirty = true;
            }
        } else {
            storeMisses++;
            totalCycles += 100;
            if (writeAllocate) {
                load(address);
                int32_t updatedBlockIndex = isHit(index, tag);
                if (updatedBlockIndex != -1) {
                    store(address);
                }
            }
        }
    }

    void runSimulation() {
        for (const auto& trace : memoryTraces) {
            int operation = trace.first;
            uint32_t address = trace.second;
            if (operation == 0) {
                load(address);
            } else if (operation == 1) {
                store(address);
            }
        }
    }
};

int main() {
    int nSets, nBlocks, blockSize, isWriteAllocate, isWriteThrough, isLRU;
    std::vector<std::pair<int, uint32_t>> fileData;

    // Read cache parameters and memory traces from a file or standard input

    // Initialize the cache simulator with the read parameters and traces
    CacheSimulatorAlternative cacheSimulator(nSets, nBlocks, blockSize, isWriteAllocate, isWriteThrough, isLRU, fileData);

    // Run the cache simulation
    cacheSimulator.runSimulation();

    // Print statistics
    cacheSimulator.printCounts();

    return 0;
}
