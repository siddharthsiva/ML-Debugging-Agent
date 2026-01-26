#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>

namespace snowflake {

// Data types supported by Mini-Snowflake
enum class DataType {
    INT32,
    INT64,
    FLOAT32,
    FLOAT64,
    STRING,
    BOOLEAN,
    TIMESTAMP
};

// Value type that can hold any supported data type
using Value = std::variant<
    int32_t,
    int64_t,
    float,
    double,
    std::string,
    bool,
    uint64_t  // timestamp as unix epoch
>;

// Column schema definition
struct ColumnSchema {
    std::string name;
    DataType type;
    bool nullable = true;
    bool primary_key = false;
    std::optional<Value> default_value;
};

// Table schema containing multiple columns
struct TableSchema {
    std::string name;
    std::vector<ColumnSchema> columns;
    std::vector<std::string> partition_keys;
    std::string storage_format = "PARQUET";  // PARQUET, COLUMNAR, COMPRESSED
    
    // Get column index by name
    std::optional<size_t> getColumnIndex(const std::string& column_name) const;
    
    // Validate a row against this schema
    bool validateRow(const std::unordered_map<std::string, Value>& row) const;
};

// Statistics for query optimization
struct ColumnStats {
    size_t row_count = 0;
    std::optional<Value> min_value;
    std::optional<Value> max_value;
    size_t null_count = 0;
    size_t distinct_count = 0;
    bool has_stats = false;
};

// Represents a single row of data
using Row = std::unordered_map<std::string, Value>;

// Batch of rows for efficient processing
struct RowBatch {
    std::vector<Row> rows;
    size_t batch_size() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
};

// Utility functions for working with Values
std::string valueToString(const Value& value);
DataType getValueType(const Value& value);
bool compareValues(const Value& a, const Value& b, const std::string& op);
Value parseValue(const std::string& str, DataType type);

} // namespace snowflake