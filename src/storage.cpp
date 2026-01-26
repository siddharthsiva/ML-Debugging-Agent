#include "storage.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <set>
#include <functional>
#include <functional>

namespace snowflake {

ColumnarStorage::ColumnarStorage(const std::string& column_path, DataType type, 
                               CompressionType compression)
    : column_path_(column_path), data_type_(type), compression_(compression) {
    
    // Ensure directory exists
    std::filesystem::create_directories(std::filesystem::path(column_path_).parent_path());
}

void ColumnarStorage::writeValues(const std::vector<Value>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add to write buffer
    write_buffer_.insert(write_buffer_.end(), values.begin(), values.end());
    
    // Flush if buffer is full
    if (write_buffer_.size() >= BUFFER_SIZE) {
        flushBuffer();
    }
    
    // Invalidate stats cache
    cached_stats_.reset();
}

void ColumnarStorage::flushBuffer() {
    if (write_buffer_.empty()) return;
    
    // Serialize values to binary format
    std::vector<uint8_t> data;
    
    for (const auto& value : write_buffer_) {
        std::visit([&data](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                // String: length (4 bytes) + data
                uint32_t len = v.size();
                data.insert(data.end(), reinterpret_cast<const uint8_t*>(&len), 
                           reinterpret_cast<const uint8_t*>(&len) + sizeof(len));
                data.insert(data.end(), v.begin(), v.end());
            } else {
                // Fixed-size types
                data.insert(data.end(), reinterpret_cast<const uint8_t*>(&v),
                           reinterpret_cast<const uint8_t*>(&v) + sizeof(v));
            }
        }, value);
    }
    
    // Compress if enabled
    if (compression_ != CompressionType::NONE) {
        data = compressData(data);
    }
    
    // Write to file (append mode)
    std::ofstream file(column_path_, std::ios::binary | std::ios::app);
    if (!file) {
        throw std::runtime_error("Failed to open column file: " + column_path_);
    }
    
    // Write compressed flag and data size
    uint8_t compressed = (compression_ != CompressionType::NONE) ? 1 : 0;
    uint32_t data_size = data.size();
    uint32_t value_count = write_buffer_.size();
    
    file.write(reinterpret_cast<const char*>(&compressed), 1);
    file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    file.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    write_buffer_.clear();
}

std::vector<Value> ColumnarStorage::readAllValues() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Flush any pending writes first
    if (!write_buffer_.empty()) {
        const_cast<ColumnarStorage*>(this)->flushBuffer();
    }
    
    std::vector<Value> result;
    
    if (!std::filesystem::exists(column_path_)) {
        return result;
    }
    
    std::ifstream file(column_path_, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open column file: " + column_path_);
    }
    
    // Read all chunks
    while (file.good() && file.peek() != EOF) {
        uint8_t compressed;
        uint32_t data_size, value_count;
        
        file.read(reinterpret_cast<char*>(&compressed), 1);
        file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
        file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
        
        if (!file) break;
        
        std::vector<uint8_t> data(data_size);
        file.read(reinterpret_cast<char*>(data.data()), data_size);
        
        // Decompress if needed
        if (compressed) {
            data = decompressData(data);
        }
        
        // Deserialize values
        size_t offset = 0;
        for (uint32_t i = 0; i < value_count && offset < data.size(); ++i) {
            switch (data_type_) {
                case DataType::INT32: {
                    int32_t val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
                case DataType::INT64: {
                    int64_t val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
                case DataType::FLOAT32: {
                    float val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
                case DataType::FLOAT64: {
                    double val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
                case DataType::STRING: {
                    uint32_t len;
                    std::memcpy(&len, data.data() + offset, sizeof(len));
                    offset += sizeof(len);
                    
                    std::string val(reinterpret_cast<const char*>(data.data() + offset), len);
                    result.emplace_back(std::move(val));
                    offset += len;
                    break;
                }
                case DataType::BOOLEAN: {
                    bool val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
                case DataType::TIMESTAMP: {
                    uint64_t val;
                    std::memcpy(&val, data.data() + offset, sizeof(val));
                    result.emplace_back(val);
                    offset += sizeof(val);
                    break;
                }
            }
        }
    }
    
    return result;
}

std::vector<Value> ColumnarStorage::readValues(size_t start_row, size_t count) {
    auto all_values = readAllValues();
    
    if (start_row >= all_values.size()) {
        return {};
    }
    
    size_t end_row = std::min(start_row + count, all_values.size());
    return std::vector<Value>(all_values.begin() + start_row, all_values.begin() + end_row);
}

size_t ColumnarStorage::getRowCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = write_buffer_.size();
    
    if (!std::filesystem::exists(column_path_)) {
        return count;
    }
    
    std::ifstream file(column_path_, std::ios::binary);
    if (!file) return count;
    
    while (file.good() && file.peek() != EOF) {
        uint8_t compressed;
        uint32_t data_size, value_count;
        
        file.read(reinterpret_cast<char*>(&compressed), 1);
        file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
        file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
        
        if (!file) break;
        
        file.seekg(data_size, std::ios::cur); // Skip data
        count += value_count;
    }
    
    return count;
}

ColumnStats ColumnarStorage::getStats() {
    if (cached_stats_.has_value()) {
        return cached_stats_.value();
    }
    
    ColumnStats stats;
    auto values = readAllValues();
    
    if (values.empty()) {
        cached_stats_ = stats;
        return stats;
    }
    
    stats.row_count = values.size();
    stats.has_stats = true;
    
    // Calculate min/max and distinct count (simplified)
    std::set<std::string> distinct_values;
    
    for (const auto& value : values) {
        std::string str_val = valueToString(value);
        distinct_values.insert(str_val);
        
        // Update min/max
        if (!stats.min_value.has_value() || compareValues(value, stats.min_value.value(), "<")) {
            stats.min_value = value;
        }
        if (!stats.max_value.has_value() || compareValues(value, stats.max_value.value(), ">")) {
            stats.max_value = value;
        }
    }
    
    stats.distinct_count = distinct_values.size();
    cached_stats_ = stats;
    
    return stats;
}

void ColumnarStorage::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    flushBuffer();
}

std::vector<uint8_t> ColumnarStorage::compressData(const std::vector<uint8_t>& data) {
    // Simplified compression (would use real compression library in production)
    // For now, just return original data
    return data;
}

std::vector<uint8_t> ColumnarStorage::decompressData(const std::vector<uint8_t>& data) {
    // Simplified decompression
    return data;
}

// TableStorage implementation

TableStorage::TableStorage(const std::string& table_path, const TableSchema& schema)
    : table_path_(table_path), schema_(schema) {
    
    std::filesystem::create_directories(table_path_);
    initializeColumns();
}

void TableStorage::initializeColumns() {
    for (const auto& column : schema_.columns) {
        auto storage = std::make_unique<ColumnarStorage>(
            getColumnPath(column.name), 
            column.type
        );
        columns_[column.name] = std::move(storage);
    }
}

std::string TableStorage::getColumnPath(const std::string& column_name, size_t partition) {
    return table_path_ + "/" + column_name + "_" + std::to_string(partition) + ".col";
}

void TableStorage::insertRow(const Row& row) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!schema_.validateRow(row)) {
        throw std::runtime_error("Row validation failed");
    }
    
    // Insert values into each column
    for (const auto& column : schema_.columns) {
        std::vector<Value> values;
        
        auto it = row.find(column.name);
        if (it != row.end()) {
            values.push_back(it->second);
        } else if (column.default_value.has_value()) {
            values.push_back(column.default_value.value());
        } else {
            // Use default zero value for the type
            switch (column.type) {
                case DataType::INT32: values.emplace_back(int32_t(0)); break;
                case DataType::INT64: values.emplace_back(int64_t(0)); break;
                case DataType::FLOAT32: values.emplace_back(0.0f); break;
                case DataType::FLOAT64: values.emplace_back(0.0); break;
                case DataType::STRING: values.emplace_back(std::string("")); break;
                case DataType::BOOLEAN: values.emplace_back(false); break;
                case DataType::TIMESTAMP: values.emplace_back(uint64_t(0)); break;
            }
        }
        
        columns_[column.name]->writeValues(values);
    }
}

void TableStorage::insertBatch(const RowBatch& batch) {
    for (const auto& row : batch.rows) {
        insertRow(row);
    }
}

RowBatch TableStorage::scanTable(const std::vector<std::string>& columns,
                                const std::function<bool(const Row&)>& predicate) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> scan_columns = columns.empty() ? 
        [this]() {
            std::vector<std::string> all_cols;
            for (const auto& col : schema_.columns) {
                all_cols.push_back(col.name);
            }
            return all_cols;
        }() : columns;
    
    // Read all required columns
    std::unordered_map<std::string, std::vector<Value>> column_data;
    size_t row_count = 0;
    
    for (const auto& col_name : scan_columns) {
        auto it = columns_.find(col_name);
        if (it != columns_.end()) {
            auto values = it->second->readAllValues();
            column_data[col_name] = std::move(values);
            row_count = std::max(row_count, column_data[col_name].size());
        }
    }
    
    // Reconstruct rows
    RowBatch result;
    for (size_t i = 0; i < row_count; ++i) {
        Row row;
        for (const auto& col_name : scan_columns) {
            if (column_data[col_name].size() > i) {
                row[col_name] = column_data[col_name][i];
            }
        }
        
        if (!predicate || predicate(row)) {
            result.rows.push_back(std::move(row));
        }
    }
    
    return result;
}

size_t TableStorage::getRowCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!columns_.empty()) {
        return columns_.begin()->second->getRowCount();
    }
    return 0;
}

std::unordered_map<std::string, ColumnStats> TableStorage::getTableStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, ColumnStats> stats;
    for (const auto& [col_name, storage] : columns_) {
        stats[col_name] = storage->getStats();
    }
    return stats;
}

void TableStorage::optimizePartitions() {
    // TODO: Implement partition optimization
}

void TableStorage::vacuum() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [col_name, storage] : columns_) {
        storage->flush();
    }
}

void TableStorage::analyze() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Force statistics recalculation
    for (const auto& [col_name, storage] : columns_) {
        storage->getStats();
    }
}

} // namespace snowflake