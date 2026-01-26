#pragma once

#include "types.h"
#include "advanced_features.h"
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

namespace snowflake {

// Real-time streaming data ingestion
class StreamingIngestion {
private:
    struct StreamBuffer {
        std::vector<Row> buffer;
        std::mutex mutex;
        std::chrono::steady_clock::time_point last_flush;
        size_t max_size;
    };
    
    std::unordered_map<std::string, std::unique_ptr<StreamBuffer>> table_buffers_;
    std::thread flush_thread_;
    std::atomic<bool> running_;
    std::chrono::milliseconds flush_interval_;
    class Catalog& catalog_;
    
    void flush_worker();
    void flush_table_buffer(const std::string& table_name);
    
public:
    explicit StreamingIngestion(class Catalog& catalog, 
                               std::chrono::milliseconds flush_interval = std::chrono::milliseconds(1000));
    ~StreamingIngestion();
    
    void start();
    void stop();
    void ingest_row(const std::string& table_name, const Row& row);
    void ingest_batch(const std::string& table_name, const std::vector<Row>& rows);
    void force_flush(const std::string& table_name = "");
    
    struct IngestionStats {
        size_t total_rows_ingested = 0;
        size_t total_batches_processed = 0;
        size_t pending_rows = 0;
        double ingestion_rate_per_second = 0.0;
        std::chrono::milliseconds avg_flush_time{0};
    };
    
    IngestionStats get_stats() const;
};

// Materialized view management
class MaterializedViewManager {
private:
    struct MaterializedView {
        std::string name;
        std::string sql_definition;
        std::string base_table;
        std::chrono::steady_clock::time_point last_refresh;
        bool auto_refresh;
        std::chrono::seconds refresh_interval;
        QueryResult cached_result;
    };
    
    std::unordered_map<std::string, MaterializedView> views_;
    std::thread refresh_thread_;
    std::atomic<bool> running_;
    mutable std::shared_mutex mutex_;
    class QueryEngine& query_engine_;
    
    void refresh_worker();
    void refresh_view(const std::string& view_name);
    
public:
    explicit MaterializedViewManager(class QueryEngine& engine);
    ~MaterializedViewManager();
    
    void start();
    void stop();
    
    void create_view(const std::string& name, const std::string& sql, 
                    const std::string& base_table, bool auto_refresh = true,
                    std::chrono::seconds refresh_interval = std::chrono::seconds(300));
    void drop_view(const std::string& name);
    void refresh_view_now(const std::string& name);
    
    QueryResult query_view(const std::string& name);
    std::vector<std::string> list_views() const;
    
    void invalidate_views_for_table(const std::string& table_name);
};

// Machine learning integration for query optimization
class MLQueryOptimizer {
private:
    struct QueryFeatures {
        size_t table_count;
        size_t join_count;
        size_t where_clause_complexity;
        size_t projected_columns;
        size_t aggregation_count;
        bool has_order_by;
        bool has_group_by;
        bool has_limit;
        double estimated_selectivity;
        size_t table_size_mb;
    };
    
    struct PerformanceData {
        QueryFeatures features;
        std::chrono::milliseconds execution_time;
        size_t rows_scanned;
        size_t memory_used_mb;
        bool used_index;
    };
    
    std::vector<PerformanceData> training_data_;
    mutable std::shared_mutex mutex_;
    bool model_trained_;
    
    QueryFeatures extract_features(const std::string& sql, class Catalog& catalog);
    double predict_execution_time(const QueryFeatures& features);
    std::string suggest_optimization(const QueryFeatures& features);
    
public:
    MLQueryOptimizer();
    
    void record_query_performance(const std::string& sql, 
                                 std::chrono::milliseconds execution_time,
                                 size_t rows_scanned, size_t memory_used,
                                 bool used_index, class Catalog& catalog);
    
    void train_model();
    
    struct OptimizationSuggestion {
        double predicted_time_ms;
        std::string suggestion;
        double confidence;
        std::vector<std::string> recommended_indexes;
    };
    
    OptimizationSuggestion analyze_query(const std::string& sql, class Catalog& catalog);
    
    void export_model(const std::string& filename);
    void import_model(const std::string& filename);
};

// Distributed query coordinator
class DistributedCoordinator {
private:
    struct QueryFragment {
        std::string sql;
        std::string target_node;
        std::vector<std::string> dependencies;
        bool completed;
        QueryResult result;
    };
    
    struct ExecutionPlan {
        std::vector<QueryFragment> fragments;
        std::string coordinator_node;
        std::chrono::steady_clock::time_point start_time;
    };
    
    LoadBalancer& load_balancer_;
    std::unordered_map<std::string, ExecutionPlan> active_queries_;
    mutable std::shared_mutex mutex_;
    
    ExecutionPlan create_execution_plan(const std::string& sql);
    void execute_fragment(QueryFragment& fragment);
    QueryResult merge_results(const std::vector<QueryResult>& partial_results);
    
public:
    explicit DistributedCoordinator(LoadBalancer& lb);
    
    QueryResult execute_distributed_query(const std::string& sql);
    void cancel_query(const std::string& query_id);
    std::vector<std::string> get_active_queries() const;
    
    struct DistributedStats {
        size_t total_distributed_queries = 0;
        size_t active_queries = 0;
        double avg_parallelism_factor = 0.0;
        std::chrono::milliseconds avg_coordination_overhead{0};
    };
    
    DistributedStats get_stats() const;
};

// Advanced compression engine
class CompressionEngine {
private:
    struct CompressionStats {
        size_t original_size;
        size_t compressed_size;
        CompressionAlgorithm algorithm;
        std::chrono::microseconds compression_time;
        std::chrono::microseconds decompression_time;
    };
    
    std::unordered_map<CompressionAlgorithm, CompressionStats> algorithm_stats_;
    mutable std::shared_mutex mutex_;
    
public:
    CompressionEngine();
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data, CompressionAlgorithm algorithm);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data, CompressionAlgorithm algorithm);
    
    // Adaptive compression - chooses best algorithm based on data characteristics
    struct AdaptiveResult {
        std::vector<uint8_t> compressed_data;
        CompressionAlgorithm chosen_algorithm;
        double compression_ratio;
        std::chrono::microseconds compression_time;
    };
    
    AdaptiveResult adaptive_compress(const std::vector<uint8_t>& data, DataType data_type);
    
    // Specialized encodings
    std::vector<uint8_t> delta_encode(const std::vector<int64_t>& data);
    std::vector<int64_t> delta_decode(const std::vector<uint8_t>& encoded);
    
    std::vector<uint8_t> rle_encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> rle_decode(const std::vector<uint8_t>& encoded);
    
    std::vector<uint8_t> dictionary_encode(const std::vector<std::string>& data);
    std::vector<std::string> dictionary_decode(const std::vector<uint8_t>& encoded);
    
    CompressionStats get_stats(CompressionAlgorithm algorithm) const;
    CompressionAlgorithm recommend_algorithm(DataType data_type, size_t data_size) const;
};

// Time series analysis for performance monitoring
class TimeSeriesAnalyzer {
private:
    struct DataPoint {
        std::chrono::steady_clock::time_point timestamp;
        double value;
        std::unordered_map<std::string, std::string> labels;
    };
    
    std::unordered_map<std::string, std::vector<DataPoint>> series_;
    mutable std::shared_mutex mutex_;
    
public:
    void add_point(const std::string& series_name, double value, 
                  const std::unordered_map<std::string, std::string>& labels = {});
    
    struct Trend {
        double slope;
        double correlation;
        bool increasing;
        double confidence;
    };
    
    Trend analyze_trend(const std::string& series_name, std::chrono::minutes window) const;
    
    struct Anomaly {
        std::chrono::steady_clock::time_point timestamp;
        double value;
        double deviation_score;
        std::string description;
    };
    
    std::vector<Anomaly> detect_anomalies(const std::string& series_name, 
                                         std::chrono::minutes window,
                                         double threshold = 3.0) const;
    
    double predict_next_value(const std::string& series_name, std::chrono::minutes lookback) const;
    
    std::vector<DataPoint> get_series(const std::string& series_name, 
                                     std::chrono::steady_clock::time_point start,
                                     std::chrono::steady_clock::time_point end) const;
};

// GPU acceleration interface
class GPUAccelerator {
private:
    bool cuda_available_;
    bool opencl_available_;
    size_t gpu_memory_mb_;
    
public:
    GPUAccelerator();
    ~GPUAccelerator();
    
    bool is_available() const { return cuda_available_ || opencl_available_; }
    
    // GPU-accelerated operations
    std::vector<double> gpu_aggregate_sum(const std::vector<double>& data);
    std::vector<bool> gpu_filter_comparison(const std::vector<int64_t>& data, 
                                           const std::string& op, int64_t value);
    std::vector<uint64_t> gpu_hash_strings(const std::vector<std::string>& data);
    std::vector<size_t> gpu_sort_indices(const std::vector<double>& data);
    
    // Machine learning on GPU
    void gpu_train_linear_regression(const std::vector<std::vector<double>>& features,
                                    const std::vector<double>& targets,
                                    std::vector<double>& weights);
    
    double gpu_predict_linear(const std::vector<double>& features,
                             const std::vector<double>& weights);
    
    struct GPUStats {
        size_t total_memory_mb;
        size_t used_memory_mb;
        double utilization_percent;
        size_t operations_completed;
        std::chrono::microseconds total_compute_time;
    };
    
    GPUStats get_stats() const;
};

} // namespace snowflake