#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "cache_sim.h"

void cacheSimulation(u_int32_t sets, u_int32_t blocks, u_int32_t bytes, std::string writeAllocate, std::string writeThrough, std::string eviction);


int main(int argc, char* argv[]) {
    if (argc < 6 || argc > 7) {
        cerr << "Invalid arguments" << endl;
        return 1;
    }

    // Read memory trace data
    std::vector<pair<int, uint32_t>> file_data;
    std::string input;
    while (getInput(cin, input)) {
        // Read a line of input and split it into operation and address
        istringstream ss(input);
        string operation, address_str; 
        ss >> operation >> address_str;
        // Check if the operation is valid
        if (operation != "s" && operation != "l") {
            cerr << "Invalid operation: " << operation << endl;
            return 1;
        }
        uint32_t address;
        if (address_str.size() > 2 && address_str.substr(0, 2) == "0x") {
            // Convert hexadecimal address to an integer
            address = stoul(address_str.substr(2), nullptr, 16);
        } else {
            cerr << "Invalid address format: " << address_str << endl;
            return 1;
        }
        // Add the operation and address to the file_data vector
        file_data.emplace_back(operation == "s", address);
    }

    int sets, blocks, block_size;
    try {
        sets = stoi(argv[1]);
        blocks = stoi(argv[2]);
        block_size = stoi(argv[3]);
    } catch (const exception& e) {
        cerr << "Invalid cache configuration" << endl;
        return 1;
    }

    if (sets <= 0 || (sets & (sets - 1)) != 0
        || blocks <= 0 || (blocks & (blocks - 1)) != 0
        || block_size < 4 || (block_size & (block_size - 1)) != 0) {
        cerr << "Invalid cache configuration" << endl;
        return 1;
    }

    // Determine cache policies
    bool writeAllocate = (string(argv[4]) == "write-allocate");
    bool writeThrough = (string(argv[5]) == "write-through");

    if (!is_write_allocate && !is_write_through) {
        cerr << "Invalid cache configuration" << endl;
        return 1;
    }

    int is_lru = -1;
    if (argc > 6) {
        if (string(argv[6]) == "lru") {
            is_lru = 1;
        } else if (string(argv[6]) == "fifo") {
            is_lru = 0;
        } else {
            cerr << "Invalid eviction policy" << endl;
            return 1;
        }
    }

    CacheSimulator cache(sets, blocks, block_size, writeAllocate, writeThrough, is_lru, file_data);
    cache.run_simulation();

    return 0;
}
