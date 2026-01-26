#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <functional>
#include <optional>
#include <set>

namespace snowflake {

// SQL AST Node Types
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Expression nodes
struct ColumnRef : ASTNode {
    std::string table_name; // optional
    std::string column_name;
};

struct Literal : ASTNode {
    Value value;
};

struct BinaryOp : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string operator_; // =, <, >, <=, >=, !=, AND, OR
};

struct FunctionCall : ASTNode {
    std::string function_name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
};

// Statement nodes
struct SelectStatement : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> select_list;
    std::string from_table;
    std::unique_ptr<ASTNode> where_clause;
    std::vector<std::string> group_by;
    std::unique_ptr<ASTNode> having_clause;
    std::vector<std::pair<std::string, bool>> order_by; // column, ascending
    std::optional<size_t> limit;
    std::optional<size_t> offset;
};

struct InsertStatement : ASTNode {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::vector<std::unique_ptr<ASTNode>>> values;
};

struct CreateTableStatement : ASTNode {
    std::string table_name;
    std::vector<ColumnSchema> columns;
    std::vector<std::string> partition_by;
    bool if_not_exists = false;
};

struct DropTableStatement : ASTNode {
    std::string table_name;
    bool if_exists = false;
};

// Advanced SQL parser with modern C++ features
class SQLParser {
private:
    std::string sql_;
    size_t pos_;
    std::vector<std::string> tokens_;
    
    // Tokenization
    void tokenize();
    bool isKeyword(const std::string& token) const;
    bool isOperator(const std::string& token) const;
    
    // Parsing utilities
    std::string currentToken() const;
    std::string consumeToken();
    bool match(const std::string& expected);
    bool matchKeyword(const std::string& keyword);
    void expect(const std::string& expected);
    void expectKeyword(const std::string& keyword);
    bool hasMoreTokens() const;
    
    // Expression parsing with precedence climbing
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parsePrimaryExpression();
    std::unique_ptr<ASTNode> parseBinaryExpression(int min_precedence);
    int getOperatorPrecedence(const std::string& op) const;
    
    // Statement parsing
    std::unique_ptr<ASTNode> parseSelect();
    std::unique_ptr<ASTNode> parseInsert();
    std::unique_ptr<ASTNode> parseCreateTable();
    std::unique_ptr<ASTNode> parseDropTable();
    
    // Helper methods
    std::vector<std::string> parseColumnList();
    DataType parseDataType(const std::string& type_str);
    
public:
    explicit SQLParser(const std::string& sql);
    
    std::unique_ptr<ASTNode> parse();
    
    // Static utility methods
    static std::vector<std::string> splitSQL(const std::string& sql);
    static bool isValidSQL(const std::string& sql);
};

// SQL validation and optimization hints
struct QueryPlan {
    std::string operation_type;
    std::string table_name;
    std::vector<std::string> projected_columns;
    std::vector<std::string> filter_conditions;
    bool uses_index = false;
    size_t estimated_rows = 0;
    double estimated_cost = 0.0;
};

class QueryOptimizer {
private:
    class Catalog& catalog_;
    
public:
    explicit QueryOptimizer(class Catalog& catalog);
    
    // Optimize query and generate execution plan
    QueryPlan optimizeQuery(const ASTNode& ast);
    
    // Predicate pushdown optimization
    std::unique_ptr<ASTNode> pushdownPredicates(std::unique_ptr<ASTNode> ast);
    
    // Column projection optimization
    std::vector<std::string> determineRequiredColumns(const ASTNode& ast);
    
    // Cost estimation
    double estimateQueryCost(const QueryPlan& plan);
};

} // namespace snowflake