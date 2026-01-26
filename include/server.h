#pragma once

#include "query_engine.h"
#include <string>
#include <thread>
#include <memory>

namespace snowflake {

// HTTP Request structure
struct HTTPRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::unordered_map<std::string, std::string> query_params;
};

// HTTP Response structure
struct HTTPResponse {
    int status_code = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    HTTPResponse() {
        headers["Content-Type"] = "application/json";
        headers["Server"] = "Mini-Snowflake/1.0";
    }
};

// JSON utilities
class JSON {
public:
    static std::string serialize(const QueryResult& result);
    static std::string serialize(const std::vector<Catalog::TableInfo>& tables);
    static std::string serialize(const TableSchema& schema);
    static std::string serializeError(const std::string& message);
    
    static TableSchema parseTableSchema(const std::string& json);
    static std::vector<Row> parseRows(const std::string& json);
};

// Simple HTTP server for Mini-Snowflake
class HTTPServer {
private:
    int port_;
    bool running_;
    std::thread server_thread_;
    QueryEngine& engine_;
    
    // Socket handling
    int server_socket_;
    void startListening();
    void handleConnection(int client_socket);
    
    // HTTP protocol
    HTTPRequest parseRequest(const std::string& raw_request);
    std::string formatResponse(const HTTPResponse& response);
    
    // Route handlers
    HTTPResponse handleQuery(const HTTPRequest& request);
    HTTPResponse handleCreateTable(const HTTPRequest& request);
    HTTPResponse handleInsert(const HTTPRequest& request);
    HTTPResponse handleListTables(const HTTPRequest& request);
    HTTPResponse handleTableSchema(const HTTPRequest& request);
    HTTPResponse handleHealth(const HTTPRequest& request);
    HTTPResponse handleMetrics(const HTTPRequest& request);
    
    // Utility methods
    HTTPResponse routeRequest(const HTTPRequest& request);
    HTTPResponse createErrorResponse(int status, const std::string& message);
    
public:
    HTTPServer(int port, QueryEngine& engine);
    ~HTTPServer();
    
    void start();
    void stop();
    bool isRunning() const { return running_; }
};

// REST API endpoints documentation:
/*
POST /api/v1/query
{
    "sql": "SELECT * FROM users WHERE age > 25"
}

POST /api/v1/tables
{
    "name": "users",
    "schema": {
        "columns": [
            {"name": "id", "type": "INT64", "primary_key": true},
            {"name": "name", "type": "STRING"},
            {"name": "age", "type": "INT32"}
        ]
    }
}

POST /api/v1/tables/{table_name}/data
{
    "rows": [
        {"id": 1, "name": "Alice", "age": 30},
        {"id": 2, "name": "Bob", "age": 25}
    ]
}

GET /api/v1/tables
GET /api/v1/tables/{table_name}/schema
GET /api/v1/health
GET /api/v1/metrics
*/

} // namespace snowflake