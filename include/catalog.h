#pragma once

#include "types.h"
#include "storage.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <filesystem>

namespace snowflake {

// Database catalog for managing tables and schemas
class Catalog {
private:
    std::string catalog_path_;
    std::string data_path_;
    std::unordered_map<std::string, TableSchema> table_schemas_;
    std::unordered_map<std::string, std::unique_ptr<TableStorage>> table_storage_;
    mutable std::shared_mutex mutex_;
    
    // Catalog persistence
    void saveCatalog();
    void loadCatalog();
    std::string getCatalogFilePath() const;
    std::string getTablePath(const std::string& table_name) const;
    
public:
    explicit Catalog(const std::string& data_path = "./data");
    ~Catalog();
    
    // Table management
    void createTable(const std::string& name, const TableSchema& schema);
    void dropTable(const std::string& name);
    bool tableExists(const std::string& name) const;
    
    // Get table operations
    TableStorage* getTable(const std::string& name);
    TableSchema getTableSchema(const std::string& name) const;
    std::vector<std::string> listTables() const;
    
    // Database operations
    void createDatabase(const std::string& name);
    void dropDatabase(const std::string& name);
    std::vector<std::string> listDatabases() const;
    
    // Statistics and information schema
    struct TableInfo {
        std::string name;
        std::string type = "TABLE";
        size_t row_count = 0;
        size_t size_bytes = 0;
        std::string created_time;
        std::string modified_time;
    };
    
    std::vector<TableInfo> getTablesInfo() const;
    
    // Maintenance
    void vacuum(); // Cleanup and optimize all tables
    void analyze(); // Update statistics for all tables
};

// Global catalog instance management
class CatalogManager {
private:
    static std::unique_ptr<Catalog> instance_;
    static std::mutex instance_mutex_;
    
public:
    static Catalog& getInstance(const std::string& data_path = "./data");
    static void shutdown();
};

} // namespace snowflake