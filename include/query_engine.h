#pragma once

#include "types.h"
#include "parser.h"
#include "catalog.h"
#include <memory>
#include <chrono>

namespace snowflake {

// Query execution result
struct QueryResult {
    std::vector<std::string> column_names;
    std::vector<Row> rows;
    size_t rows_affected = 0;
    std::chrono::milliseconds execution_time{0};
    std::string message;
    bool success = true;
    
    // Performance metrics
    size_t rows_scanned = 0;
    size_t bytes_scanned = 0;
    bool used_pushdown = false;
};

// Abstract executor interface
class Executor {
public:
    virtual ~Executor() = default;
    virtual QueryResult execute() = 0;
    virtual std::string getOperatorName() const = 0;
};

// Scan executor with predicate pushdown
class ScanExecutor : public Executor {
private:
    TableStorage* table_;
    std::vector<std::string> columns_;
    std::function<bool(const Row&)> predicate_;
    std::string table_name_;
    
public:
    ScanExecutor(TableStorage* table, const std::string& table_name,
                std::vector<std::string> columns = {},
                std::function<bool(const Row&)> predicate = nullptr);
    
    QueryResult execute() override;
    std::string getOperatorName() const override { return "TableScan"; }
};

// Filter executor
class FilterExecutor : public Executor {
private:
    std::unique_ptr<Executor> child_;
    std::function<bool(const Row&)> predicate_;
    
public:
    FilterExecutor(std::unique_ptr<Executor> child,
                  std::function<bool(const Row&)> predicate);
    
    QueryResult execute() override;
    std::string getOperatorName() const override { return "Filter"; }
};

// Projection executor
class ProjectExecutor : public Executor {
private:
    std::unique_ptr<Executor> child_;
    std::vector<std::string> columns_;
    
public:
    ProjectExecutor(std::unique_ptr<Executor> child,
                   std::vector<std::string> columns);
    
    QueryResult execute() override;
    std::string getOperatorName() const override { return "Project"; }
};

// Sort executor
class SortExecutor : public Executor {
private:
    std::unique_ptr<Executor> child_;
    std::vector<std::pair<std::string, bool>> sort_keys_; // column, ascending
    
public:
    SortExecutor(std::unique_ptr<Executor> child,
                std::vector<std::pair<std::string, bool>> sort_keys);
    
    QueryResult execute() override;
    std::string getOperatorName() const override { return "Sort"; }
};

// Limit executor
class LimitExecutor : public Executor {
private:
    std::unique_ptr<Executor> child_;
    size_t limit_;
    size_t offset_;
    
public:
    LimitExecutor(std::unique_ptr<Executor> child, size_t limit, size_t offset = 0);
    
    QueryResult execute() override;
    std::string getOperatorName() const override { return "Limit"; }
};

// Main query engine
class QueryEngine {
private:
    Catalog& catalog_;
    QueryOptimizer optimizer_;
    
    // Expression evaluation
    Value evaluateExpression(const ASTNode& expr, const Row& row);
    std::function<bool(const Row&)> compilePredicateToFunction(const ASTNode& predicate);
    
    // Executor building from AST
    std::unique_ptr<Executor> buildSelectExecutor(const SelectStatement& stmt);
    QueryResult executeInsert(const InsertStatement& stmt);
    QueryResult executeCreateTable(const CreateTableStatement& stmt);
    QueryResult executeDropTable(const DropTableStatement& stmt);
    
public:
    explicit QueryEngine(Catalog& catalog);
    
    // Main query execution interface
    QueryResult executeSQL(const std::string& sql);
    QueryResult executeAST(const ASTNode& ast);
    
    // Batch execution
    std::vector<QueryResult> executeBatch(const std::vector<std::string>& sql_statements);
    
    // Query analysis
    QueryPlan explain(const std::string& sql);
    std::string getQueryPlan(const std::string& sql);
    
    // Performance monitoring
    struct EngineStats {
        size_t queries_executed = 0;
        size_t total_execution_time_ms = 0;
        size_t total_rows_scanned = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
    };
    
    EngineStats getStats() const { return stats_; }
    void resetStats() { stats_ = {}; }
    
private:
    EngineStats stats_;
};

} // namespace snowflake