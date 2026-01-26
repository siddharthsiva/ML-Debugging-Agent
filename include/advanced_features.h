#pragma once

#include "types.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>

namespace snowflake {

// Advanced compression and encoding algorithms
enum class CompressionAlgorithm {
    NONE,
    LZ4,
    SNAPPY,
    ZSTD,
    GZIP,
    RLE,           // Run-length encoding for repetitive data
    DELTA,         // Delta encoding for timestamps/sequences
    DICTIONARY,    // Dictionary encoding for strings
    BITMAP,        // Bitmap encoding for booleans
    FRAME_OF_REF   // Frame of reference for integers
};

enum class ColumnEncoding {
    PLAIN,
    DELTA_BINARY_PACKED,
    DELTA_LENGTH_BYTE_ARRAY,
    RLE_DICTIONARY,
    BYTE_STREAM_SPLIT,
    ADAPTIVE_ENCODING
};

// Advanced vectorized operations
class VectorizedProcessor {
public:
    // SIMD-optimized operations
    static void vectorized_add_int64(const int64_t* a, const int64_t* b, int64_t* result, size_t count);
    static void vectorized_filter_greater(const int64_t* data, const int64_t threshold, bool* result, size_t count);
    static void vectorized_hash_string(const std::string* data, uint64_t* hashes, size_t count);
    static void vectorized_aggregate_sum(const double* data, const bool* filter, double& result, size_t count);
    
    // Advanced string operations
    static void vectorized_like_match(const std::string* data, const std::string& pattern, bool* result, size_t count);
    static void vectorized_regex_match(const std::string* data, const std::string& regex, bool* result, size_t count);
};

// Bloom filter for efficient existence checks
class BloomFilter {
private:
    std::vector<uint64_t> bits_;
    size_t num_hash_functions_;
    size_t size_bits_;
    
public:
    BloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
    
    void add(const std::string& item);
    void add(uint64_t hash);
    bool might_contain(const std::string& item) const;
    bool might_contain(uint64_t hash) const;
    
    void clear();
    double current_false_positive_rate() const;
    size_t memory_usage() const;
};

// Zone maps for min/max pruning
struct ZoneMap {
    Value min_value;
    Value max_value;
    size_t null_count = 0;
    size_t row_count = 0;
    std::unique_ptr<BloomFilter> bloom_filter;
    
    bool can_prune_with_predicate(const std::string& op, const Value& value) const;
    void update_with_value(const Value& value);
    void merge_with(const ZoneMap& other);
};

// Advanced column statistics for query optimization
struct AdvancedColumnStats : public ColumnStats {
    std::vector<Value> histogram_bounds;
    std::vector<size_t> histogram_counts;
    double cardinality_estimate = 0.0;
    std::unordered_map<std::string, size_t> top_k_values;
    std::unique_ptr<BloomFilter> bloom_filter;
    std::vector<ZoneMap> zone_maps;  // One per data block
    
    // Sketch-based statistics
    double hyperloglog_cardinality = 0.0;
    std::vector<uint8_t> count_min_sketch;
    
    void update_histogram(const std::vector<Value>& values);
    double estimate_selectivity(const std::string& op, const Value& value) const;
    size_t estimate_result_size(const std::string& op, const Value& value, size_t total_rows) const;
};

// Memory pool for efficient allocation
class MemoryPool {
private:
    struct Block {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        size_t used;
    };
    
    std::vector<Block> blocks_;
    size_t default_block_size_;
    mutable std::mutex mutex_;
    
public:
    explicit MemoryPool(size_t default_block_size = 64 * 1024 * 1024); // 64MB default
    
    void* allocate(size_t size, size_t alignment = 8);
    void reset();
    size_t total_allocated() const;
    size_t total_used() const;
};

// Lock-free hash table for high-performance lookups
template<typename K, typename V>
class LockFreeHashTable {
private:
    struct Entry {
        std::atomic<K> key;
        std::atomic<V> value;
        std::atomic<bool> valid;
    };
    
    std::vector<Entry> table_;
    size_t capacity_;
    std::hash<K> hasher_;
    
public:
    explicit LockFreeHashTable(size_t capacity);
    
    bool insert(const K& key, const V& value);
    bool find(const K& key, V& value) const;
    bool remove(const K& key);
    double load_factor() const;
};

// Connection pool for database connections
class ConnectionPool {
private:
    struct Connection {
        int socket_fd;
        std::chrono::steady_clock::time_point last_used;
        bool in_use;
    };
    
    std::vector<Connection> connections_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_connections_;
    std::chrono::seconds connection_timeout_;
    
public:
    explicit ConnectionPool(size_t max_connections = 100, 
                           std::chrono::seconds timeout = std::chrono::seconds(300));
    ~ConnectionPool();
    
    int acquire_connection();
    void release_connection(int socket_fd);
    void cleanup_idle_connections();
    size_t active_connections() const;
    size_t total_connections() const;
};

// Adaptive query cache with LRU eviction
class QueryCache {
private:
    struct CacheEntry {
        std::string sql_hash;
        QueryResult result;
        std::chrono::steady_clock::time_point timestamp;
        size_t access_count;
        size_t size_bytes;
    };
    
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache_;
    std::list<std::string> lru_list_;
    mutable std::shared_mutex mutex_;
    size_t max_size_bytes_;
    size_t current_size_bytes_;
    
    void evict_lru();
    std::string hash_query(const std::string& sql) const;
    
public:
    explicit QueryCache(size_t max_size_bytes = 1024 * 1024 * 1024); // 1GB default
    
    bool get(const std::string& sql, QueryResult& result);
    void put(const std::string& sql, const QueryResult& result);
    void invalidate(const std::string& table_name);
    void clear();
    
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        size_t size_bytes = 0;
        size_t entry_count = 0;
        double hit_rate = 0.0;
    };
    
    CacheStats get_stats() const;
};

// Real-time metrics collection
class MetricsCollector {
private:
    struct Metric {
        std::string name;
        double value;
        std::chrono::steady_clock::time_point timestamp;
        std::unordered_map<std::string, std::string> labels;
    };
    
    std::vector<Metric> metrics_;
    mutable std::shared_mutex mutex_;
    std::chrono::seconds retention_period_;
    
public:
    explicit MetricsCollector(std::chrono::seconds retention = std::chrono::seconds(3600));
    
    void record_counter(const std::string& name, double value = 1.0, 
                       const std::unordered_map<std::string, std::string>& labels = {});
    void record_gauge(const std::string& name, double value,
                     const std::unordered_map<std::string, std::string>& labels = {});
    void record_histogram(const std::string& name, double value,
                         const std::unordered_map<std::string, std::string>& labels = {});
    
    std::vector<Metric> get_metrics(const std::string& name_filter = "") const;
    std::string export_prometheus() const;
    void cleanup_old_metrics();
};

// Load balancer for distributed queries
class LoadBalancer {
public:
    enum class Strategy {
        ROUND_ROBIN,
        LEAST_CONNECTIONS,
        WEIGHTED_ROUND_ROBIN,
        CONSISTENT_HASHING,
        RESOURCE_AWARE
    };
    
private:
    struct Node {
        std::string address;
        int port;
        double weight;
        size_t active_connections;
        std::chrono::steady_clock::time_point last_health_check;
        bool healthy;
        double cpu_usage;
        double memory_usage;
    };
    
    std::vector<Node> nodes_;
    Strategy strategy_;
    std::atomic<size_t> round_robin_index_;
    mutable std::shared_mutex mutex_;
    
public:
    explicit LoadBalancer(Strategy strategy = Strategy::ROUND_ROBIN);
    
    void add_node(const std::string& address, int port, double weight = 1.0);
    void remove_node(const std::string& address, int port);
    Node* select_node(const std::string& key = "");
    void update_node_health(const std::string& address, int port, bool healthy);
    void update_node_resources(const std::string& address, int port, double cpu, double memory);
    
    std::vector<Node> get_healthy_nodes() const;
    size_t total_active_connections() const;
};

} // namespace snowflake