#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <set>
#include <functional>

namespace snowflake {

// Compression algorithms
enum class CompressionType {
    NONE,
    SNAPPY,
    LZ4,
    GZIP
};

// Abstract base class for column storage
class ColumnStorage {
public:
    virtual ~ColumnStorage() = default;
    
    // Write values to storage
    virtual void writeValues(const std::vector<Value>& values) = 0;
    
    // Read all values from storage
    virtual std::vector<Value> readAllValues() = 0;
    
    // Read values in a specific range
    virtual std::vector<Value> readValues(size_t start_row, size_t count) = 0;
    
    // Get number of rows stored
    virtual size_t getRowCount() const = 0;
    
    // Get column statistics
    virtual ColumnStats getStats() = 0;
    
    // Flush any pending writes
    virtual void flush() = 0;
};

// High-performance columnar storage implementation
class ColumnarStorage : public ColumnStorage {
private:
    std::string column_path_;
    DataType data_type_;
    CompressionType compression_;
    mutable std::mutex mutex_;
    
    // In-memory buffer for writes
    std::vector<Value> write_buffer_;
    static constexpr size_t BUFFER_SIZE = 10000;
    
    // Statistics cache
    mutable std::optional<ColumnStats> cached_stats_;
    
    // File handling
    void flushBuffer();
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& data);
    
public:
    ColumnarStorage(const std::string& column_path, DataType type, 
                   CompressionType compression = CompressionType::LZ4);
    
    void writeValues(const std::vector<Value>& values) override;
    std::vector<Value> readAllValues() override;
    std::vector<Value> readValues(size_t start_row, size_t count) override;
    size_t getRowCount() const override;
    ColumnStats getStats() override;
    void flush() override;
};

// Table storage manager
class TableStorage {
private:
    std::string table_path_;
    TableSchema schema_;
    std::unordered_map<std::string, std::unique_ptr<ColumnStorage>> columns_;
    mutable std::mutex mutex_;
    
    // Partition management
    std::vector<std::string> partitions_;
    size_t current_partition_ = 0;
    static constexpr size_t MAX_PARTITION_SIZE = 1000000; // 1M rows per partition
    
    void initializeColumns();
    std::string getColumnPath(const std::string& column_name, size_t partition = 0);
    
public:
    TableStorage(const std::string& table_path, const TableSchema& schema);
    
    // Insert operations
    void insertRow(const Row& row);
    void insertBatch(const RowBatch& batch);
    
    // Query operations with predicate pushdown
    RowBatch scanTable(const std::vector<std::string>& columns = {},
                      const std::function<bool(const Row&)>& predicate = nullptr);
    
    // Statistics and metadata
    size_t getRowCount() const;
    std::unordered_map<std::string, ColumnStats> getTableStats();
    TableSchema getSchema() const { return schema_; }
    
    // Partition management
    void optimizePartitions();
    std::vector<std::string> getPartitions() const { return partitions_; }
    
    // Maintenance operations
    void vacuum(); // Cleanup deleted rows and optimize storage
    void analyze(); // Update statistics
};

} // namespace snowflake