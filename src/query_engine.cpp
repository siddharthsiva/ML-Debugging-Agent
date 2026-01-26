#include "query_engine.h"
#include "catalog.h"
#include <chrono>
#include <algorithm>

namespace snowflake {

// QueryEngine implementation

QueryEngine::QueryEngine(Catalog& catalog) 
    : catalog_(catalog), optimizer_(catalog) {
}

QueryResult QueryEngine::executeSQL(const std::string& sql) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        SQLParser parser(sql);
        auto ast = parser.parse();
        
        auto result = executeAST(*ast);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        // Update engine stats
        stats_.queries_executed++;
        stats_.total_execution_time_ms += result.execution_time.count();
        stats_.total_rows_scanned += result.rows_scanned;
        
        return result;
        
    } catch (const std::exception& e) {
        QueryResult result;
        result.success = false;
        result.message = e.what();
        return result;
    }
}

QueryResult QueryEngine::executeAST(const ASTNode& ast) {
    // Determine AST node type and execute accordingly
    if (const auto* select_stmt = dynamic_cast<const SelectStatement*>(&ast)) {
        auto executor = buildSelectExecutor(*select_stmt);
        return executor->execute();
    }
    
    if (const auto* insert_stmt = dynamic_cast<const InsertStatement*>(&ast)) {
        return executeInsert(*insert_stmt);
    }
    
    if (const auto* create_stmt = dynamic_cast<const CreateTableStatement*>(&ast)) {
        return executeCreateTable(*create_stmt);
    }
    
    if (const auto* drop_stmt = dynamic_cast<const DropTableStatement*>(&ast)) {
        return executeDropTable(*drop_stmt);
    }
    
    QueryResult result;
    result.success = false;
    result.message = "Unsupported AST node type";
    return result;
}

std::unique_ptr<Executor> QueryEngine::buildSelectExecutor(const SelectStatement& stmt) {
    // Start with table scan
    auto* table = catalog_.getTable(stmt.from_table);
    
    // Determine required columns for projection
    std::vector<std::string> required_columns;
    for (const auto& select_expr : stmt.select_list) {
        if (const auto* col_ref = dynamic_cast<const ColumnRef*>(select_expr.get())) {
            required_columns.push_back(col_ref->column_name);
        }
    }
    
    // Build predicate function if WHERE clause exists
    std::function<bool(const Row&)> predicate = nullptr;
    if (stmt.where_clause) {
        predicate = compilePredicateToFunction(*stmt.where_clause);
    }
    
    // Create scan executor with predicate pushdown
    auto executor = std::make_unique<ScanExecutor>(
        table, stmt.from_table, required_columns, predicate);
    
    std::unique_ptr<Executor> current = std::move(executor);
    
    // Add projection if needed (all columns vs specific columns)
    if (!required_columns.empty()) {
        current = std::make_unique<ProjectExecutor>(std::move(current), required_columns);
    }
    
    // Add sorting if ORDER BY exists
    if (!stmt.order_by.empty()) {
        current = std::make_unique<SortExecutor>(std::move(current), stmt.order_by);
    }
    
    // Add limit if specified
    if (stmt.limit.has_value()) {
        current = std::make_unique<LimitExecutor>(
            std::move(current), stmt.limit.value(), stmt.offset.value_or(0));
    }
    
    return current;
}

std::function<bool(const Row&)> QueryEngine::compilePredicateToFunction(const ASTNode& predicate) {
    return [this, &predicate](const Row& row) -> bool {
        try {
            Value result = evaluateExpression(predicate, row);
            
            // Convert result to boolean
            return std::visit([](const auto& v) -> bool {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, bool>) {
                    return v;
                } else if constexpr (std::is_arithmetic_v<T>) {
                    return v != 0;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return !v.empty();
                }
                return false;
            }, result);
            
        } catch (const std::exception&) {
            return false; // Failed evaluation means predicate is false
        }
    };
}

Value QueryEngine::evaluateExpression(const ASTNode& expr, const Row& row) {
    if (const auto* col_ref = dynamic_cast<const ColumnRef*>(&expr)) {
        auto it = row.find(col_ref->column_name);
        if (it != row.end()) {
            return it->second;
        }
        throw std::runtime_error("Column not found: " + col_ref->column_name);
    }
    
    if (const auto* literal = dynamic_cast<const Literal*>(&expr)) {
        return literal->value;
    }
    
    if (const auto* binary_op = dynamic_cast<const BinaryOp*>(&expr)) {
        Value left = evaluateExpression(*binary_op->left, row);
        Value right = evaluateExpression(*binary_op->right, row);
        
        // Handle logical operators
        if (binary_op->operator_ == "AND") {
            bool left_bool = std::get<bool>(left);
            bool right_bool = std::get<bool>(right);
            return left_bool && right_bool;
        }
        
        if (binary_op->operator_ == "OR") {
            bool left_bool = std::get<bool>(left);
            bool right_bool = std::get<bool>(right);
            return left_bool || right_bool;
        }
        
        // Handle comparison operators
        return compareValues(left, right, binary_op->operator_);
    }
    
    if (const auto* func_call = dynamic_cast<const FunctionCall*>(&expr)) {
        // Handle function calls (simplified)
        if (func_call->function_name == "COUNT") {
            return static_cast<int64_t>(1); // COUNT(*) implementation
        }
    }
    
    throw std::runtime_error("Unsupported expression type");
}

QueryResult QueryEngine::executeInsert(const InsertStatement& stmt) {
    QueryResult result;
    
    try {
        auto* table = catalog_.getTable(stmt.table_name);
        
        size_t rows_inserted = 0;
        for (const auto& value_list : stmt.values) {
            Row row;
            
            // Build row from values
            for (size_t i = 0; i < stmt.columns.size() && i < value_list.size(); ++i) {
                if (const auto* literal = dynamic_cast<const Literal*>(value_list[i].get())) {
                    row[stmt.columns[i]] = literal->value;
                }
            }
            
            table->insertRow(row);
            rows_inserted++;
        }
        
        result.rows_affected = rows_inserted;
        result.message = "Inserted " + std::to_string(rows_inserted) + " rows";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
    }
    
    return result;
}

QueryResult QueryEngine::executeCreateTable(const CreateTableStatement& stmt) {
    QueryResult result;
    
    try {
        TableSchema schema;
        schema.name = stmt.table_name;
        schema.columns = stmt.columns;
        schema.partition_keys = stmt.partition_by;
        
        catalog_.createTable(stmt.table_name, schema);
        
        result.message = "Table '" + stmt.table_name + "' created successfully";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
    }
    
    return result;
}

QueryResult QueryEngine::executeDropTable(const DropTableStatement& stmt) {
    QueryResult result;
    
    try {
        catalog_.dropTable(stmt.table_name);
        result.message = "Table '" + stmt.table_name + "' dropped successfully";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
    }
    
    return result;
}

std::vector<QueryResult> QueryEngine::executeBatch(const std::vector<std::string>& sql_statements) {
    std::vector<QueryResult> results;
    
    for (const auto& sql : sql_statements) {
        results.push_back(executeSQL(sql));
    }
    
    return results;
}

QueryPlan QueryEngine::explain(const std::string& sql) {
    try {
        SQLParser parser(sql);
        auto ast = parser.parse();
        return optimizer_.optimizeQuery(*ast);
    } catch (const std::exception&) {
        return QueryPlan{}; // Empty plan on parse failure
    }
}

std::string QueryEngine::getQueryPlan(const std::string& sql) {
    auto plan = explain(sql);
    
    std::stringstream ss;
    ss << "Query Plan for: " << sql << "\n";
    ss << "Operation: " << plan.operation_type << "\n";
    ss << "Table: " << plan.table_name << "\n";
    ss << "Projected Columns: ";
    for (const auto& col : plan.projected_columns) {
        ss << col << " ";
    }
    ss << "\nFilter Conditions: ";
    for (const auto& filter : plan.filter_conditions) {
        ss << filter << " ";
    }
    ss << "\nEstimated Rows: " << plan.estimated_rows << "\n";
    ss << "Estimated Cost: " << plan.estimated_cost << "\n";
    
    return ss.str();
}

// Executor implementations

ScanExecutor::ScanExecutor(TableStorage* table, const std::string& table_name,
                          std::vector<std::string> columns,
                          std::function<bool(const Row&)> predicate)
    : table_(table), table_name_(table_name), columns_(std::move(columns)), 
      predicate_(std::move(predicate)) {
}

QueryResult ScanExecutor::execute() {
    QueryResult result;
    
    try {
        auto batch = table_->scanTable(columns_, predicate_);
        
        result.rows = std::move(batch.rows);
        result.rows_scanned = result.rows.size();
        
        // Set column names
        if (!result.rows.empty()) {
            for (const auto& [col_name, value] : result.rows[0]) {
                result.column_names.push_back(col_name);
            }
            std::sort(result.column_names.begin(), result.column_names.end());
        }
        
        result.used_pushdown = (predicate_ != nullptr);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
    }
    
    return result;
}

FilterExecutor::FilterExecutor(std::unique_ptr<Executor> child,
                              std::function<bool(const Row&)> predicate)
    : child_(std::move(child)), predicate_(std::move(predicate)) {
}

QueryResult FilterExecutor::execute() {
    auto child_result = child_->execute();
    
    if (!child_result.success) {
        return child_result;
    }
    
    QueryResult result = child_result;
    result.rows.clear();
    
    for (const auto& row : child_result.rows) {
        if (predicate_(row)) {
            result.rows.push_back(row);
        }
    }
    
    return result;
}

ProjectExecutor::ProjectExecutor(std::unique_ptr<Executor> child,
                                std::vector<std::string> columns)
    : child_(std::move(child)), columns_(std::move(columns)) {
}

QueryResult ProjectExecutor::execute() {
    auto child_result = child_->execute();
    
    if (!child_result.success) {
        return child_result;
    }
    
    QueryResult result = child_result;
    result.column_names = columns_;
    
    // Project only requested columns
    for (auto& row : result.rows) {
        Row projected_row;
        for (const auto& col : columns_) {
            auto it = row.find(col);
            if (it != row.end()) {
                projected_row[col] = it->second;
            }
        }
        row = std::move(projected_row);
    }
    
    return result;
}

SortExecutor::SortExecutor(std::unique_ptr<Executor> child,
                          std::vector<std::pair<std::string, bool>> sort_keys)
    : child_(std::move(child)), sort_keys_(std::move(sort_keys)) {
}

QueryResult SortExecutor::execute() {
    auto child_result = child_->execute();
    
    if (!child_result.success) {
        return child_result;
    }
    
    QueryResult result = child_result;
    
    // Sort rows based on sort keys
    std::sort(result.rows.begin(), result.rows.end(),
        [this](const Row& a, const Row& b) -> bool {
            for (const auto& [col_name, ascending] : sort_keys_) {
                auto it_a = a.find(col_name);
                auto it_b = b.find(col_name);
                
                if (it_a == a.end() || it_b == b.end()) {
                    continue;
                }
                
                bool less_than = compareValues(it_a->second, it_b->second, "<");
                bool greater_than = compareValues(it_a->second, it_b->second, ">");
                
                if (less_than) {
                    return ascending;
                } else if (greater_than) {
                    return !ascending;
                }
                // Equal, continue to next sort key
            }
            return false; // Equal on all keys
        });
    
    return result;
}

LimitExecutor::LimitExecutor(std::unique_ptr<Executor> child, size_t limit, size_t offset)
    : child_(std::move(child)), limit_(limit), offset_(offset) {
}

QueryResult LimitExecutor::execute() {
    auto child_result = child_->execute();
    
    if (!child_result.success) {
        return child_result;
    }
    
    QueryResult result = child_result;
    
    // Apply offset and limit
    if (offset_ >= result.rows.size()) {
        result.rows.clear();
    } else {
        size_t start = offset_;
        size_t end = std::min(start + limit_, result.rows.size());
        
        std::vector<Row> limited_rows(
            result.rows.begin() + start,
            result.rows.begin() + end
        );
        
        result.rows = std::move(limited_rows);
    }
    
    return result;
}

} // namespace snowflake