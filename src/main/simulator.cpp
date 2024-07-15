// Lie Leon Alexius
#include <systemc>

// Request and Result struct
#include "simulator.hpp"
#include "../modules/modules.hpp"
#include "parser/parse.h"

extern "C" {
    #include "grapher/printer.h"
}

// prevent the C++ compiler from mangling the function name
extern "C" {
    Config* config = nullptr;

    /**
     * @brief Set the config
     * @author Lie Leon Alexius
     */
    void set_config(Config* c) {
        std::cout << "Fetching config..." << std::endl;
        config = c;
    }

    /**
     * @brief For testing the request
     * 
     * @details
     * "src/assets/csv/test_valid.csv"
     * Adr: 1 Data: 0 WE: 0
     * Adr: 2 Data: 100 WE: 1
     * Adr: 3 Data: 0 WE: 0
     * Adr: 4 Data: 100 WE: 1
     * Adr: 5 Data: 0 WE: 0
     * Adr: 6 Data: 200 WE: 1
     * Adr: 2 Data: 0 WE: 0
     * Adr: 3 Data: 0 WE: 0
     * Adr: 3 Data: 300 WE: 1
     * Adr: 4 Data: 0 WE: 0
     * Adr: 0 Data: 0 WE: -1
     * 
     * @author Lie Leon Alexius
     */
    void print_requests(size_t numRequests, struct Request* requests) {
        for (size_t i = 0; i < numRequests; i++) {
            struct Request r = requests[i];
            std::cout << "Adr: " << r.addr << " Data: " << r.data << " WE: " << r.we << std::endl;

            // stop printing
            if (r.we == -1) {
                break;
            }
        }  
    }
    
    /**
     * @brief updates the CacheSimulatorStats
     * @author Lie Leon Alexius
     */
    void statsUpdater(CacheStats* cacheStats, CacheStats tempStats) {
        cacheStats->cycles += tempStats.cycles;
        cacheStats->misses += tempStats.misses;
        cacheStats->hits += tempStats.hits;
        cacheStats->read_hits += tempStats.read_hits;
        cacheStats->read_misses += tempStats.read_misses;
        cacheStats->write_hits += tempStats.write_hits;
        cacheStats->write_misses += tempStats.write_misses;
        cacheStats->read_hits_L1 += tempStats.read_hits_L1;
        cacheStats->read_misses_L1 += tempStats.read_misses_L1;
        cacheStats->write_hits_L1 += tempStats.write_hits_L1;
        cacheStats->write_misses_L1 += tempStats.write_misses_L1;
        cacheStats->read_hits_L2 += tempStats.read_hits_L2;
        cacheStats->read_misses_L2 += tempStats.read_misses_L2;
        cacheStats->write_hits_L2 += tempStats.write_hits_L2;
        cacheStats->write_misses_L2 += tempStats.write_misses_L2;
    }

    /**
     * @brief Runs the cache simulation
     *
     * @param cycles The number of cycles for the simulation. Default value is `1000000`.
     * @param l1CacheLines The number of lines in L1 cache. Default value is `64`.
     * @param l2CacheLines The number of lines in L2 cache. Default value is `256`.
     * @param cacheLineSize The cache line size in bytes. Default value is `64`.
     * @param l1CacheLatency The latency for L1 cache in cycles. Default value is `4`.
     * @param l2CacheLatency The latency for L2 cache in cycles. Default value is `12`.
     * @param memoryLatency The latency for memory access in cycles. Default value is `100`.
     * @param numRequests The number of requests. Default value is `1000`.
     * @param requests A pointer to the array of Request structures.
     * @param tracefile The name of the trace file. Default value is "default_trace.vcd".
     * 
     * @return Result struct
     * 
     * @warning Not tested yet
     * @bug Not tested yet
     * 
     * @todo Cycle breaker needs to be adjusted (due to cache.finish_memory())
     * 
     * @authors
     * Lie Leon Alexius
     * Anthony Tang
     */
    Result* run_simulation(
        int cycles, 
        unsigned l1CacheLines, unsigned l2CacheLines, unsigned cacheLineSize, 
        unsigned l1CacheLatency, unsigned l2CacheLatency, unsigned memoryLatency, 
        size_t numRequests, struct Request* requests,
        const char* tracefile
    ) 
    {
        // Test the Requests
        // print_requests(numRequests, requests);

        // Get the config (Optimization flags)
        unsigned int prefetchBuffer = 0; 
        unsigned int storebackBuffer = 0; 
        bool storebackBufferCondition= false;

        if (config != nullptr) {
            std::cout << "Using config..." << std::endl;
            prefetchBuffer = config->prefetchBuffer;
            storebackBuffer = config->storebackBuffer;
            storebackBufferCondition = config->storebackBufferCondition;
        }

        // Initialize the cache simulator       
        CPU_L1_L2 caches(
            l1CacheLines, l2CacheLines, cacheLineSize, 
            l1CacheLatency, l2CacheLatency, memoryLatency, 
            tracefile, 
            prefetchBuffer, storebackBuffer, storebackBufferCondition
        );

        // Initialize the cacheStats
        CacheStats* cacheStats = (CacheStats*) malloc(sizeof(CacheStats));
        cacheStats->cycles = 0;
        cacheStats->misses = 0;
        cacheStats->hits = 0;
        cacheStats->read_hits = 0;
        cacheStats->read_misses = 0;
        cacheStats->write_hits = 0;
        cacheStats->write_misses = 0;
        cacheStats->read_hits_L1 = 0;
        cacheStats->read_misses_L1 = 0;
        cacheStats->write_hits_L1 = 0;
        cacheStats->write_misses_L1 = 0;
        cacheStats->read_hits_L2 = 0;
        cacheStats->read_misses_L2 = 0;
        cacheStats->write_hits_L2 = 0;
        cacheStats->write_misses_L2 = 0;

        // ========================================================================================
        
        std::cout << "Running simulation..." << std::endl;

        // Process the request
        for (size_t i = 0; i < numRequests; i++) {
            struct Request req = requests[i];

            // If req.we == -1, end simulation
            if (req.we == -1) {
                break;
            }
            
            // Send request to cache
            CacheStats tempResult = caches.send_request(req);

            // break if total simulated cache will be higher than limit
            if (cacheStats->cycles + tempResult.cycles > cycles) {
                break;
            }

            // update the cacheStats
            statsUpdater(cacheStats, tempResult);
        }

        // All request has been sent, wait until the simulation finishes
        unsigned int memory_cycles = caches.finish_memory();
        cacheStats->cycles += memory_cycles;

        // ========================================================================================
        // assign Result
        Result* result = (Result*) malloc(sizeof(Result));
        result->cycles = cacheStats->cycles;
        result->hits = cacheStats->hits;
        result->misses = cacheStats->misses;
        result->primitiveGateCount = caches.get_gate_count(); // fetch the gate count
        result->cacheStats = cacheStats;

        // stop the simulation and close the trace file
        (tracefile != nullptr) ? caches.close_trace_file() : caches.stop_simulation();

        // Case this function is not called by "executor.c"
        if (config == nullptr) {
            // re-create config
            config = (Config*) malloc(sizeof(Config));
            config->cycles = cycles;
            config->l1CacheLines = l1CacheLines;
            config->l2CacheLines = l2CacheLines;
            config->cacheLineSize = cacheLineSize;
            config->l1CacheLatency = l1CacheLatency;
            config->l2CacheLatency = l2CacheLatency;
            config->memoryLatency = memoryLatency;
            config->numRequests = numRequests;
            config->tracefile = nullptr;
            config->input_filename = nullptr;
            config->requests = nullptr;
            config->customNumRequest = false;
            config->prefetchBuffer = prefetchBuffer;
            config->storebackBuffer = storebackBuffer;
            config->storebackBufferCondition = storebackBufferCondition;
            config->prettyPrint = true;

            std::cout << "Simulator is not called from executor.c" << std::endl;

            print_layout(config, result);
        }
        
        // return the result
        return result;
    }
}

// The default sc_main implementation.
int sc_main(int argc, char* argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}
