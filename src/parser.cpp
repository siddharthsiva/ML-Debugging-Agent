#include "parser.h"
#include "types.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <set>
#include <stdexcept>
#include <iostream>

namespace snowflake {

SQLParser::SQLParser(const std::string& sql) : sql_(sql), pos_(0) {
    tokenize();
}

void SQLParser::tokenize() {
    std::string current_token;
    bool in_string = false;
    char string_delimiter = '\0';
    
    for (size_t i = 0; i < sql_.length(); ++i) {
        char c = sql_[i];
        
        if (in_string) {
            current_token += c;
            if (c == string_delimiter) {
                in_string = false;
                tokens_.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
        
        if (c == '\'' || c == '"') {
            if (!current_token.empty()) {
                tokens_.push_back(current_token);
                current_token.clear();
            }
            current_token += c;
            in_string = true;
            string_delimiter = c;
            continue;
        }
        
        if (std::isspace(c)) {
            if (!current_token.empty()) {
                tokens_.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
        
        if (c == '(' || c == ')' || c == ',' || c == ';' || c == '*') {
            if (!current_token.empty()) {
                tokens_.push_back(current_token);
                current_token.clear();
            }
            tokens_.push_back(std::string(1, c));
            continue;
        }
        
        // Handle multi-character operators
        if (c == '=' || c == '<' || c == '>' || c == '!') {
            if (!current_token.empty()) {
                tokens_.push_back(current_token);
                current_token.clear();
            }
            
            std::string op(1, c);
            if (i + 1 < sql_.length()) {
                char next = sql_[i + 1];
                if ((c == '<' && next == '=') || 
                    (c == '>' && next == '=') || 
                    (c == '!' && next == '=') ||
                    (c == '<' && next == '>')) {
                    op += next;
                    ++i;
                }
            }
            tokens_.push_back(op);
            continue;
        }
        
        current_token += c;
    }
    
    if (!current_token.empty()) {
        tokens_.push_back(current_token);
    }
}

std::string SQLParser::currentToken() const {
    if (pos_ >= tokens_.size()) {
        return "";
    }
    return tokens_[pos_];
}

std::string SQLParser::consumeToken() {
    if (pos_ >= tokens_.size()) {
        return "";
    }
    return tokens_[pos_++];
}

bool SQLParser::match(const std::string& expected) {
    if (currentToken() == expected) {
        consumeToken();
        return true;
    }
    return false;
}

bool SQLParser::matchKeyword(const std::string& keyword) {
    std::string current = currentToken();
    std::transform(current.begin(), current.end(), current.begin(), ::toupper);
    
    if (current == keyword) {
        consumeToken();
        return true;
    }
    return false;
}

void SQLParser::expect(const std::string& expected) {
    if (!match(expected)) {
        throw std::runtime_error("Expected '" + expected + "' but found '" + currentToken() + "'");
    }
}

void SQLParser::expectKeyword(const std::string& keyword) {
    if (!matchKeyword(keyword)) {
        throw std::runtime_error("Expected keyword '" + keyword + "' but found '" + currentToken() + "'");
    }
}

bool SQLParser::hasMoreTokens() const {
    return pos_ < tokens_.size();
}

bool SQLParser::isKeyword(const std::string& token) const {
    std::string upper_token = token;
    std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);
    
    static const std::set<std::string> keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "CREATE", "TABLE",
        "DROP", "DELETE", "UPDATE", "SET", "ORDER", "BY", "GROUP", "HAVING",
        "LIMIT", "OFFSET", "AND", "OR", "NOT", "IN", "LIKE", "BETWEEN",
        "NULL", "IS", "AS", "DISTINCT", "COUNT", "SUM", "AVG", "MAX", "MIN"
    };
    
    return keywords.find(upper_token) != keywords.end();
}

bool SQLParser::isOperator(const std::string& token) const {
    static const std::set<std::string> operators = {
        "=", "!=", "<>", "<", "<=", ">", ">=", "+", "-", "*", "/", "%"
    };
    
    return operators.find(token) != operators.end();
}

std::unique_ptr<ASTNode> SQLParser::parse() {
    if (!hasMoreTokens()) {
        throw std::runtime_error("Empty SQL statement");
    }
    
    std::string first_token = currentToken();
    std::transform(first_token.begin(), first_token.end(), first_token.begin(), ::toupper);
    
    if (first_token == "SELECT") {
        return parseSelect();
    } else if (first_token == "INSERT") {
        return parseInsert();
    } else if (first_token == "CREATE") {
        return parseCreateTable();
    } else if (first_token == "DROP") {
        return parseDropTable();
    }
    
    throw std::runtime_error("Unsupported SQL statement: " + first_token);
}

std::unique_ptr<ASTNode> SQLParser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expect("SELECT");
    
    // Parse select list
    do {
        if (match("*")) {
            // SELECT * - we'll handle this by not restricting columns
            auto col_ref = std::make_unique<ColumnRef>();
            col_ref->column_name = "*";
            stmt->select_list.push_back(std::move(col_ref));
        } else {
            auto expr = parseExpression();
            stmt->select_list.push_back(std::move(expr));
        }
    } while (match(","));
    
    // Parse FROM clause
    if (!matchKeyword("FROM")) {
        throw std::runtime_error("Missing FROM clause");
    }
    
    stmt->from_table = consumeToken();
    
    // Parse WHERE clause
    if (matchKeyword("WHERE")) {
        stmt->where_clause = parseExpression();
    }
    
    // Parse ORDER BY
    if (matchKeyword("ORDER")) {
        if (!matchKeyword("BY")) {
            throw std::runtime_error("Expected BY after ORDER");
        }
        
        do {
            std::string column = consumeToken();
            bool ascending = true;
            
            if (matchKeyword("DESC")) {
                ascending = false;
            } else {
                matchKeyword("ASC"); // Optional
            }
            
            stmt->order_by.push_back({column, ascending});
        } while (match(","));
    }
    
    // Parse LIMIT
    if (matchKeyword("LIMIT")) {
        stmt->limit = std::stoull(consumeToken());
        
        if (matchKeyword("OFFSET")) {
            stmt->offset = std::stoull(consumeToken());
        }
    }
    
    return std::move(stmt);
}

std::unique_ptr<ASTNode> SQLParser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expect("INSERT");
    expectKeyword("INTO");
    
    stmt->table_name = consumeToken();
    
    // Parse column list
    if (match("(")) {
        do {
            stmt->columns.push_back(consumeToken());
        } while (match(","));
        expect(")");
    }
    
    expectKeyword("VALUES");
    
    // Parse values
    do {
        expect("(");
        std::vector<std::unique_ptr<ASTNode>> row_values;
        
        do {
            row_values.push_back(parseExpression());
        } while (match(","));
        
        expect(")");
        stmt->values.push_back(std::move(row_values));
    } while (match(","));
    
    return std::move(stmt);
}

std::unique_ptr<ASTNode> SQLParser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expect("CREATE");
    expectKeyword("TABLE");
    
    if (matchKeyword("IF")) {
        expectKeyword("NOT");
        expectKeyword("EXISTS");
        stmt->if_not_exists = true;
    }
    
    stmt->table_name = consumeToken();
    
    expect("(");
    
    // Parse column definitions
    do {
        ColumnSchema column;
        column.name = consumeToken();
        
        std::string type_str = consumeToken();
        column.type = parseDataType(type_str);
        
        // Parse column constraints
        while (hasMoreTokens() && currentToken() != "," && currentToken() != ")") {
            if (matchKeyword("NOT")) {
                expectKeyword("NULL");
                column.nullable = false;
            } else if (matchKeyword("PRIMARY")) {
                expectKeyword("KEY");
                column.primary_key = true;
                column.nullable = false;
            } else if (matchKeyword("DEFAULT")) {
                // Parse default value
                auto default_expr = parseExpression();
                if (const auto* literal = dynamic_cast<const Literal*>(default_expr.get())) {
                    column.default_value = literal->value;
                }
            } else {
                consumeToken(); // Skip unknown constraint
            }
        }
        
        stmt->columns.push_back(std::move(column));
    } while (match(","));
    
    expect(")");
    
    return std::move(stmt);
}

std::unique_ptr<ASTNode> SQLParser::parseDropTable() {
    auto stmt = std::make_unique<DropTableStatement>();
    
    expect("DROP");
    expectKeyword("TABLE");
    
    if (matchKeyword("IF")) {
        expectKeyword("EXISTS");
        stmt->if_exists = true;
    }
    
    stmt->table_name = consumeToken();
    
    return std::move(stmt);
}

std::unique_ptr<ASTNode> SQLParser::parseExpression() {
    return parseBinaryExpression(0);
}

std::unique_ptr<ASTNode> SQLParser::parsePrimaryExpression() {
    std::string token = currentToken();
    
    // Handle parentheses
    if (match("(")) {
        auto expr = parseExpression();
        expect(")");
        return expr;
    }
    
    // Handle literals
    if (token.front() == '\'' || token.front() == '"') {
        consumeToken();
        auto literal = std::make_unique<Literal>();
        literal->value = token.substr(1, token.length() - 2); // Remove quotes
        return std::move(literal);
    }
    
    // Handle numeric literals
    if (std::isdigit(token.front()) || token.front() == '-') {
        consumeToken();
        auto literal = std::make_unique<Literal>();
        
        if (token.find('.') != std::string::npos) {
            literal->value = std::stod(token);
        } else {
            literal->value = static_cast<int64_t>(std::stoll(token));
        }
        
        return std::move(literal);
    }
    
    // Handle function calls
    if (hasMoreTokens() && tokens_[pos_ + 1] == "(") {
        std::string func_name = consumeToken();
        auto func_call = std::make_unique<FunctionCall>();
        func_call->function_name = func_name;
        
        expect("(");
        
        if (!match(")")) {
            do {
                func_call->arguments.push_back(parseExpression());
            } while (match(","));
            expect(")");
        }
        
        return std::move(func_call);
    }
    
    // Handle column references
    auto col_ref = std::make_unique<ColumnRef>();
    col_ref->column_name = consumeToken();
    
    // Handle table.column syntax
    if (match(".")) {
        col_ref->table_name = col_ref->column_name;
        col_ref->column_name = consumeToken();
    }
    
    return std::move(col_ref);
}

std::unique_ptr<ASTNode> SQLParser::parseBinaryExpression(int min_precedence) {
    auto left = parsePrimaryExpression();
    
    while (hasMoreTokens() && isOperator(currentToken())) {
        std::string op = currentToken();
        int precedence = getOperatorPrecedence(op);
        
        if (precedence < min_precedence) {
            break;
        }
        
        consumeToken(); // Consume operator
        
        auto right = parseBinaryExpression(precedence + 1);
        
        auto binary_op = std::make_unique<BinaryOp>();
        binary_op->left = std::move(left);
        binary_op->right = std::move(right);
        binary_op->operator_ = op;
        
        left = std::move(binary_op);
    }
    
    // Handle logical operators (AND, OR)
    while (hasMoreTokens()) {
        std::string upper_token = currentToken();
        std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);
        
        if (upper_token == "AND" || upper_token == "OR") {
            int precedence = (upper_token == "AND") ? 2 : 1;
            
            if (precedence < min_precedence) {
                break;
            }
            
            consumeToken();
            
            auto right = parseBinaryExpression(precedence + 1);
            
            auto binary_op = std::make_unique<BinaryOp>();
            binary_op->left = std::move(left);
            binary_op->right = std::move(right);
            binary_op->operator_ = upper_token;
            
            left = std::move(binary_op);
        } else {
            break;
        }
    }
    
    return left;
}

int SQLParser::getOperatorPrecedence(const std::string& op) const {
    if (op == "OR") return 1;
    if (op == "AND") return 2;
    if (op == "=" || op == "!=" || op == "<>" || 
        op == "<" || op == "<=" || op == ">" || op == ">=") return 3;
    if (op == "+" || op == "-") return 4;
    if (op == "*" || op == "/" || op == "%") return 5;
    
    return 0; // Unknown operator
}

std::vector<std::string> SQLParser::parseColumnList() {
    std::vector<std::string> columns;
    
    do {
        columns.push_back(consumeToken());
    } while (match(","));
    
    return columns;
}

DataType SQLParser::parseDataType(const std::string& type_str) {
    std::string upper_type = type_str;
    std::transform(upper_type.begin(), upper_type.end(), upper_type.begin(), ::toupper);
    
    if (upper_type == "INT" || upper_type == "INTEGER" || upper_type == "INT32") {
        return DataType::INT32;
    }
    if (upper_type == "BIGINT" || upper_type == "INT64") {
        return DataType::INT64;
    }
    if (upper_type == "FLOAT" || upper_type == "REAL" || upper_type == "FLOAT32") {
        return DataType::FLOAT32;
    }
    if (upper_type == "DOUBLE" || upper_type == "FLOAT64") {
        return DataType::FLOAT64;
    }
    if (upper_type == "VARCHAR" || upper_type == "TEXT" || upper_type == "STRING") {
        return DataType::STRING;
    }
    if (upper_type == "BOOLEAN" || upper_type == "BOOL") {
        return DataType::BOOLEAN;
    }
    if (upper_type == "TIMESTAMP" || upper_type == "DATETIME") {
        return DataType::TIMESTAMP;
    }
    
    throw std::runtime_error("Unsupported data type: " + type_str);
}

std::vector<std::string> SQLParser::splitSQL(const std::string& sql) {
    std::vector<std::string> statements;
    std::stringstream ss(sql);
    std::string statement;
    
    while (std::getline(ss, statement, ';')) {
        // Trim whitespace
        statement.erase(0, statement.find_first_not_of(" \t\n\r"));
        statement.erase(statement.find_last_not_of(" \t\n\r") + 1);
        
        if (!statement.empty()) {
            statements.push_back(statement);
        }
    }
    
    return statements;
}

bool SQLParser::isValidSQL(const std::string& sql) {
    try {
        SQLParser parser(sql);
        parser.parse();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// QueryOptimizer implementation

QueryOptimizer::QueryOptimizer(Catalog& catalog) : catalog_(catalog) {
}

QueryPlan QueryOptimizer::optimizeQuery(const ASTNode& ast) {
    QueryPlan plan;
    
    if (const auto* select_stmt = dynamic_cast<const SelectStatement*>(&ast)) {
        plan.operation_type = "SELECT";
        plan.table_name = select_stmt->from_table;
        
        // Determine projected columns
        for (const auto& expr : select_stmt->select_list) {
            if (const auto* col_ref = dynamic_cast<const ColumnRef*>(expr.get())) {
                if (col_ref->column_name != "*") {
                    plan.projected_columns.push_back(col_ref->column_name);
                }
            }
        }
        
        // Extract filter conditions
        if (select_stmt->where_clause) {
            plan.filter_conditions.push_back("WHERE clause present");
        }
        
        // Estimate query cost
        plan.estimated_cost = estimateQueryCost(plan);
    }
    
    return plan;
}

std::unique_ptr<ASTNode> QueryOptimizer::pushdownPredicates(std::unique_ptr<ASTNode> ast) {
    // Simplified predicate pushdown - would be more complex in real implementation
    return ast;
}

std::vector<std::string> QueryOptimizer::determineRequiredColumns(const ASTNode& ast) {
    std::vector<std::string> columns;
    
    if (const auto* select_stmt = dynamic_cast<const SelectStatement*>(&ast)) {
        for (const auto& expr : select_stmt->select_list) {
            if (const auto* col_ref = dynamic_cast<const ColumnRef*>(expr.get())) {
                if (col_ref->column_name != "*") {
                    columns.push_back(col_ref->column_name);
                }
            }
        }
    }
    
    return columns;
}

double QueryOptimizer::estimateQueryCost(const QueryPlan& plan) {
    // Simplified cost estimation
    double base_cost = 1.0;
    
    if (!plan.projected_columns.empty()) {
        base_cost *= plan.projected_columns.size();
    }
    
    if (!plan.filter_conditions.empty()) {
        base_cost *= 0.5; // Assume filters reduce cost
    }
    
    return base_cost;
}

} // namespace snowflake