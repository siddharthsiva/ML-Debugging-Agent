#include "server.h"
#include <sstream>
#include <thread>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace snowflake {

// JSON utility implementations

std::string JSON::serialize(const QueryResult& result) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
    ss << "  \"message\": \"" << result.message << "\",\n";
    ss << "  \"execution_time_ms\": " << result.execution_time.count() << ",\n";
    ss << "  \"rows_affected\": " << result.rows_affected << ",\n";
    ss << "  \"rows_scanned\": " << result.rows_scanned << ",\n";
    
    if (!result.column_names.empty()) {
        ss << "  \"columns\": [";
        for (size_t i = 0; i < result.column_names.size(); ++i) {
            ss << "\"" << result.column_names[i] << "\"";
            if (i < result.column_names.size() - 1) ss << ", ";
        }
        ss << "],\n";
    }
    
    ss << "  \"data\": [\n";
    for (size_t i = 0; i < result.rows.size(); ++i) {
        const auto& row = result.rows[i];
        ss << "    {\n";
        
        bool first = true;
        for (const auto& [col_name, value] : row) {
            if (!first) ss << ",\n";
            first = false;
            
            ss << "      \"" << col_name << "\": ";
            
            std::visit([&ss](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    ss << "\"" << v << "\"";
                } else if constexpr (std::is_same_v<T, bool>) {
                    ss << (v ? "true" : "false");
                } else {
                    ss << v;
                }
            }, value);
        }
        
        ss << "\n    }";
        if (i < result.rows.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"count\": " << result.rows.size() << "\n";
    ss << "}";
    
    return ss.str();
}

std::string JSON::serialize(const std::vector<Catalog::TableInfo>& tables) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"tables\": [\n";
    
    for (size_t i = 0; i < tables.size(); ++i) {
        const auto& table = tables[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << table.name << "\",\n";
        ss << "      \"type\": \"" << table.type << "\",\n";
        ss << "      \"row_count\": " << table.row_count << ",\n";
        ss << "      \"size_bytes\": " << table.size_bytes << ",\n";
        ss << "      \"created_time\": \"" << table.created_time << "\",\n";
        ss << "      \"modified_time\": \"" << table.modified_time << "\"\n";
        ss << "    }";
        
        if (i < tables.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}";
    
    return ss.str();
}

std::string JSON::serialize(const TableSchema& schema) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << schema.name << "\",\n";
    ss << "  \"columns\": [\n";
    
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        const auto& col = schema.columns[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << col.name << "\",\n";
        ss << "      \"type\": \"";
        
        switch (col.type) {
            case DataType::INT32: ss << "INT32"; break;
            case DataType::INT64: ss << "INT64"; break;
            case DataType::FLOAT32: ss << "FLOAT32"; break;
            case DataType::FLOAT64: ss << "FLOAT64"; break;
            case DataType::STRING: ss << "STRING"; break;
            case DataType::BOOLEAN: ss << "BOOLEAN"; break;
            case DataType::TIMESTAMP: ss << "TIMESTAMP"; break;
        }
        
        ss << "\",\n";
        ss << "      \"nullable\": " << (col.nullable ? "true" : "false") << ",\n";
        ss << "      \"primary_key\": " << (col.primary_key ? "true" : "false") << "\n";
        ss << "    }";
        
        if (i < schema.columns.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}";
    
    return ss.str();
}

std::string JSON::serializeError(const std::string& message) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"success\": false,\n";
    ss << "  \"error\": \"" << message << "\"\n";
    ss << "}";
    return ss.str();
}

TableSchema JSON::parseTableSchema(const std::string& json) {
    // Simplified JSON parsing - would use real JSON library in production
    TableSchema schema;
    
    // Extract table name (basic pattern matching)
    size_t name_pos = json.find("\"name\":");
    if (name_pos != std::string::npos) {
        size_t start_quote = json.find('"', name_pos + 7);
        size_t end_quote = json.find('"', start_quote + 1);
        if (start_quote != std::string::npos && end_quote != std::string::npos) {
            schema.name = json.substr(start_quote + 1, end_quote - start_quote - 1);
        }
    }
    
    // For demo purposes, return a simple schema
    // Real implementation would parse JSON properly
    
    return schema;
}

std::vector<Row> JSON::parseRows(const std::string& json) {
    // Simplified JSON parsing for demo
    std::vector<Row> rows;
    
    // Real implementation would parse JSON properly
    // For now, return empty vector
    
    return rows;
}

// HTTPServer implementation

HTTPServer::HTTPServer(int port, QueryEngine& engine) 
    : port_(port), running_(false), engine_(engine), server_socket_(-1) {
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
}

HTTPServer::~HTTPServer() {
    stop();
    
#ifdef _WIN32
    WSACleanup();
#endif
}

void HTTPServer::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    server_thread_ = std::thread(&HTTPServer::startListening, this);
    
    std::cout << "Mini-Snowflake HTTP Server started on port " << port_ << std::endl;
    std::cout << "API endpoints:" << std::endl;
    std::cout << "  POST /api/v1/query - Execute SQL queries" << std::endl;
    std::cout << "  POST /api/v1/tables - Create tables" << std::endl;
    std::cout << "  GET  /api/v1/tables - List tables" << std::endl;
    std::cout << "  GET  /api/v1/health - Health check" << std::endl;
}

void HTTPServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (server_socket_ != -1) {
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    std::cout << "HTTP Server stopped" << std::endl;
}

void HTTPServer::startListening() {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind socket
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    // Listen
    if (listen(server_socket_, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return;
    }
    
    // Accept connections
    while (running_) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (sockaddr*)&client_addr, &client_len);
        if (client_socket >= 0) {
            // Handle connection in separate thread
            std::thread client_thread(&HTTPServer::handleConnection, this, client_socket);
            client_thread.detach();
        }
    }
}

void HTTPServer::handleConnection(int client_socket) {
    char buffer[8192];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        try {
            HTTPRequest request = parseRequest(std::string(buffer));
            HTTPResponse response = routeRequest(request);
            std::string response_str = formatResponse(response);
            
            send(client_socket, response_str.c_str(), response_str.length(), 0);
        } catch (const std::exception& e) {
            HTTPResponse error_response = createErrorResponse(500, e.what());
            std::string response_str = formatResponse(error_response);
            send(client_socket, response_str.c_str(), response_str.length(), 0);
        }
    }
    
#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

HTTPRequest HTTPServer::parseRequest(const std::string& raw_request) {
    HTTPRequest request;
    
    std::stringstream ss(raw_request);
    std::string line;
    
    // Parse request line
    if (std::getline(ss, line)) {
        std::stringstream line_ss(line);
        line_ss >> request.method >> request.path;
    }
    
    // Parse headers
    while (std::getline(ss, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // Skip ": "
            
            // Remove \r if present
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            
            request.headers[key] = value;
        }
    }
    
    // Parse body (remaining content)
    std::string body;
    while (std::getline(ss, line)) {
        body += line + "\n";
    }
    request.body = body;
    
    return request;
}

std::string HTTPServer::formatResponse(const HTTPResponse& response) {
    std::stringstream ss;
    
    // Status line
    ss << "HTTP/1.1 " << response.status_code << " ";
    switch (response.status_code) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default: ss << "Unknown"; break;
    }
    ss << "\r\n";
    
    // Headers
    for (const auto& [key, value] : response.headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    ss << "Content-Length: " << response.body.length() << "\r\n";
    ss << "\r\n";
    
    // Body
    ss << response.body;
    
    return ss.str();
}

HTTPResponse HTTPServer::routeRequest(const HTTPRequest& request) {
    std::cout << request.method << " " << request.path << std::endl;
    
    if (request.path == "/api/v1/health") {
        return handleHealth(request);
    }
    
    if (request.path == "/api/v1/query" && request.method == "POST") {
        return handleQuery(request);
    }
    
    if (request.path == "/api/v1/tables") {
        if (request.method == "POST") {
            return handleCreateTable(request);
        } else if (request.method == "GET") {
            return handleListTables(request);
        }
    }
    
    if (request.path.find("/api/v1/tables/") == 0 && request.path.find("/schema") != std::string::npos) {
        return handleTableSchema(request);
    }
    
    if (request.path == "/api/v1/metrics") {
        return handleMetrics(request);
    }
    
    return createErrorResponse(404, "Endpoint not found");
}

HTTPResponse HTTPServer::handleHealth(const HTTPRequest& request) {
    HTTPResponse response;
    response.body = R"({"status": "healthy", "service": "mini-snowflake"})";
    return response;
}

HTTPResponse HTTPServer::handleQuery(const HTTPRequest& request) {
    try {
        // Parse SQL from JSON body (simplified)
        size_t sql_pos = request.body.find("\"sql\":");
        if (sql_pos == std::string::npos) {
            return createErrorResponse(400, "Missing 'sql' field in request body");
        }
        
        // Extract SQL query (basic parsing)
        size_t start_quote = request.body.find('"', sql_pos + 6);
        size_t end_quote = request.body.find('"', start_quote + 1);
        
        if (start_quote == std::string::npos || end_quote == std::string::npos) {
            return createErrorResponse(400, "Invalid SQL query format");
        }
        
        std::string sql = request.body.substr(start_quote + 1, end_quote - start_quote - 1);
        
        // Execute query
        QueryResult result = engine_.executeSQL(sql);
        
        HTTPResponse response;
        response.body = JSON::serialize(result);
        
        if (!result.success) {
            response.status_code = 400;
        }
        
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(500, e.what());
    }
}

HTTPResponse HTTPServer::handleCreateTable(const HTTPRequest& request) {
    try {
        // Simplified table creation - would parse JSON properly in production
        HTTPResponse response;
        response.body = R"({"message": "Table creation endpoint - implement JSON parsing"})";
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(500, e.what());
    }
}

HTTPResponse HTTPServer::handleInsert(const HTTPRequest& request) {
    try {
        HTTPResponse response;
        response.body = R"({"message": "Insert endpoint - implement JSON parsing"})";
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(500, e.what());
    }
}

HTTPResponse HTTPServer::handleListTables(const HTTPRequest& request) {
    try {
        // Get catalog instance
        auto& catalog = CatalogManager::getInstance();
        auto tables_info = catalog.getTablesInfo();
        
        HTTPResponse response;
        response.body = JSON::serialize(tables_info);
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(500, e.what());
    }
}

HTTPResponse HTTPServer::handleTableSchema(const HTTPRequest& request) {
    try {
        // Extract table name from path
        std::string path = request.path;
        size_t start = path.find("/tables/") + 8;
        size_t end = path.find("/schema");
        
        if (start == std::string::npos || end == std::string::npos) {
            return createErrorResponse(400, "Invalid table schema path");
        }
        
        std::string table_name = path.substr(start, end - start);
        
        auto& catalog = CatalogManager::getInstance();
        TableSchema schema = catalog.getTableSchema(table_name);
        
        HTTPResponse response;
        response.body = JSON::serialize(schema);
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(404, "Table not found: " + std::string(e.what()));
    }
}

HTTPResponse HTTPServer::handleMetrics(const HTTPRequest& request) {
    try {
        auto stats = engine_.getStats();
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"queries_executed\": " << stats.queries_executed << ",\n";
        ss << "  \"total_execution_time_ms\": " << stats.total_execution_time_ms << ",\n";
        ss << "  \"total_rows_scanned\": " << stats.total_rows_scanned << ",\n";
        ss << "  \"cache_hits\": " << stats.cache_hits << ",\n";
        ss << "  \"cache_misses\": " << stats.cache_misses << "\n";
        ss << "}";
        
        HTTPResponse response;
        response.body = ss.str();
        return response;
        
    } catch (const std::exception& e) {
        return createErrorResponse(500, e.what());
    }
}

HTTPResponse HTTPServer::createErrorResponse(int status, const std::string& message) {
    HTTPResponse response;
    response.status_code = status;
    response.body = JSON::serializeError(message);
    return response;
}

} // namespace snowflake