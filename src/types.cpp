#include "types.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace snowflake {

std::optional<size_t> TableSchema::getColumnIndex(const std::string& column_name) const {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == column_name) {
            return i;
        }
    }
    return std::nullopt;
}

bool TableSchema::validateRow(const std::unordered_map<std::string, Value>& row) const {
    // Check that all non-nullable columns without defaults are present
    for (const auto& col : columns) {
        if (!col.nullable && !col.default_value.has_value() && row.find(col.name) == row.end()) {
            return false;
        }
    }
    
    // Check that all provided values match expected types
    for (const auto& [col_name, value] : row) {
        auto col_idx = getColumnIndex(col_name);
        if (!col_idx.has_value()) {
            continue; // Unknown column, ignore for now
        }
        
        const auto& col_schema = columns[col_idx.value()];
        DataType expected_type = col_schema.type;
        DataType actual_type = getValueType(value);
        
        // Type compatibility check
        if (expected_type != actual_type) {
            // Allow some implicit conversions
            if (expected_type == DataType::INT64 && actual_type == DataType::INT32) {
                continue;
            }
            if (expected_type == DataType::FLOAT64 && actual_type == DataType::FLOAT32) {
                continue;
            }
            return false;
        }
    }
    
    return true;
}

std::string valueToString(const Value& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(v);
        }
        return "unknown";
    }, value);
}

DataType getValueType(const Value& value) {
    return std::visit([](const auto& v) -> DataType {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int32_t>) {
            return DataType::INT32;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return DataType::INT64;
        } else if constexpr (std::is_same_v<T, float>) {
            return DataType::FLOAT32;
        } else if constexpr (std::is_same_v<T, double>) {
            return DataType::FLOAT64;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return DataType::STRING;
        } else if constexpr (std::is_same_v<T, bool>) {
            return DataType::BOOLEAN;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return DataType::TIMESTAMP;
        }
        throw std::runtime_error("Unknown value type");
    }, value);
}

bool compareValues(const Value& a, const Value& b, const std::string& op) {
    // Handle null comparisons
    if (a.index() == 0 || b.index() == 0) {
        if (op == "=" || op == "==") {
            return a.index() == b.index(); // Both null
        }
        return false; // Any comparison with null is false except for equality
    }
    
    // Type-safe comparison
    return std::visit([&op](const auto& lhs, const auto& rhs) -> bool {
        using LType = std::decay_t<decltype(lhs)>;
        using RType = std::decay_t<decltype(rhs)>;
        
        // Same type comparison
        if constexpr (std::is_same_v<LType, RType>) {
            if (op == "=" || op == "==") return lhs == rhs;
            if (op == "!=" || op == "<>") return lhs != rhs;
            if (op == "<") return lhs < rhs;
            if (op == "<=") return lhs <= rhs;
            if (op == ">") return lhs > rhs;
            if (op == ">=") return lhs >= rhs;
        }
        
        // Cross-type numeric comparisons
        if constexpr (std::is_arithmetic_v<LType> && std::is_arithmetic_v<RType>) {
            double left = static_cast<double>(lhs);
            double right = static_cast<double>(rhs);
            
            if (op == "=" || op == "==") return left == right;
            if (op == "!=" || op == "<>") return left != right;
            if (op == "<") return left < right;
            if (op == "<=") return left <= right;
            if (op == ">") return left > right;
            if (op == ">=") return left >= right;
        }
        
        return false; // Incompatible types
    }, a, b);
}

Value parseValue(const std::string& str, DataType type) {
    switch (type) {
        case DataType::INT32:
            return static_cast<int32_t>(std::stoi(str));
        case DataType::INT64:
            return static_cast<int64_t>(std::stoll(str));
        case DataType::FLOAT32:
            return std::stof(str);
        case DataType::FLOAT64:
            return std::stod(str);
        case DataType::STRING:
            return str;
        case DataType::BOOLEAN:
            return str == "true" || str == "1" || str == "TRUE";
        case DataType::TIMESTAMP:
            return static_cast<uint64_t>(std::stoull(str));
        default:
            throw std::runtime_error("Unsupported data type");
    }
}

} // namespace snowflake