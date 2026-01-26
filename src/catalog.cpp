#include "catalog.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace snowflake {

Catalog::Catalog(const std::string& data_path) : data_path_(data_path) {
    std::filesystem::create_directories(data_path_);
    loadCatalog();
}

Catalog::~Catalog() {
    saveCatalog();
}

std::string Catalog::getCatalogFilePath() const {
    return data_path_ + "/_catalog.json";
}

std::string Catalog::getTablePath(const std::string& table_name) const {
    return data_path_ + "/tables/" + table_name;
}

void Catalog::saveCatalog() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    std::ofstream file(getCatalogFilePath());
    if (!file) {
        throw std::runtime_error("Failed to save catalog");
    }
    
    // Simple JSON serialization
    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"tables\": {\n";
    
    bool first = true;
    for (const auto& [table_name, schema] : table_schemas_) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    \"" << table_name << "\": {\n";
        file << "      \"columns\": [\n";
        
        for (size_t i = 0; i < schema.columns.size(); ++i) {
            const auto& col = schema.columns[i];
            file << "        {\n";
            file << "          \"name\": \"" << col.name << "\",\n";
            file << "          \"type\": \"";
            
            switch (col.type) {
                case DataType::INT32: file << "INT32"; break;
                case DataType::INT64: file << "INT64"; break;
                case DataType::FLOAT32: file << "FLOAT32"; break;
                case DataType::FLOAT64: file << "FLOAT64"; break;
                case DataType::STRING: file << "STRING"; break;
                case DataType::BOOLEAN: file << "BOOLEAN"; break;
                case DataType::TIMESTAMP: file << "TIMESTAMP"; break;
            }
            
            file << "\",\n";
            file << "          \"nullable\": " << (col.nullable ? "true" : "false") << ",\n";
            file << "          \"primary_key\": " << (col.primary_key ? "true" : "false") << "\n";
            file << "        }";
            
            if (i < schema.columns.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "      ]\n";
        file << "    }";
    }
    
    file << "\n  }\n";
    file << "}\n";
}

void Catalog::loadCatalog() {
    std::ifstream file(getCatalogFilePath());
    if (!file) {
        // No catalog file exists yet, start fresh
        return;
    }
    
    // Simple JSON parsing (would use proper JSON library in production)
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    // For simplicity, we'll just create empty schemas here
    // In a real implementation, you'd parse the JSON properly
    
    file.close();
}

void Catalog::createTable(const std::string& name, const TableSchema& schema) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (table_schemas_.find(name) != table_schemas_.end()) {
        throw std::runtime_error("Table already exists: " + name);
    }
    
    // Create table storage
    auto table_storage = std::make_unique<TableStorage>(getTablePath(name), schema);
    
    // Add to catalog
    table_schemas_[name] = schema;
    table_storage_[name] = std::move(table_storage);
    
    saveCatalog();
}

void Catalog::dropTable(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = table_schemas_.find(name);
    if (it == table_schemas_.end()) {
        throw std::runtime_error("Table does not exist: " + name);
    }
    
    // Remove from memory
    table_schemas_.erase(it);
    table_storage_.erase(name);
    
    // Remove files (simplified - would need recursive directory removal)
    std::filesystem::remove_all(getTablePath(name));
    
    saveCatalog();
}

bool Catalog::tableExists(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return table_schemas_.find(name) != table_schemas_.end();
}

TableStorage* Catalog::getTable(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = table_storage_.find(name);
    if (it != table_storage_.end()) {
        return it->second.get();
    }
    
    // Try to load table if schema exists but storage doesn't
    auto schema_it = table_schemas_.find(name);
    if (schema_it != table_schemas_.end()) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(mutex_);
        
        // Double-check after acquiring write lock
        auto storage_it = table_storage_.find(name);
        if (storage_it == table_storage_.end()) {
            auto table_storage = std::make_unique<TableStorage>(
                getTablePath(name), schema_it->second);
            auto* ptr = table_storage.get();
            table_storage_[name] = std::move(table_storage);
            return ptr;
        } else {
            return storage_it->second.get();
        }
    }
    
    throw std::runtime_error("Table does not exist: " + name);
}

TableSchema Catalog::getTableSchema(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = table_schemas_.find(name);
    if (it == table_schemas_.end()) {
        throw std::runtime_error("Table does not exist: " + name);
    }
    
    return it->second;
}

std::vector<std::string> Catalog::listTables() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::string> tables;
    for (const auto& [name, schema] : table_schemas_) {
        tables.push_back(name);
    }
    
    std::sort(tables.begin(), tables.end());
    return tables;
}

void Catalog::createDatabase(const std::string& name) {
    // Simplified - just create a directory
    std::filesystem::create_directories(data_path_ + "/databases/" + name);
}

void Catalog::dropDatabase(const std::string& name) {
    std::filesystem::remove_all(data_path_ + "/databases/" + name);
}

std::vector<std::string> Catalog::listDatabases() const {
    std::vector<std::string> databases;
    std::string db_path = data_path_ + "/databases";
    
    if (std::filesystem::exists(db_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(db_path)) {
            if (entry.is_directory()) {
                databases.push_back(entry.path().filename().string());
            }
        }
    }
    
    std::sort(databases.begin(), databases.end());
    return databases;
}

std::vector<Catalog::TableInfo> Catalog::getTablesInfo() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<TableInfo> tables_info;
    
    for (const auto& [name, schema] : table_schemas_) {
        TableInfo info;
        info.name = name;
        info.type = "TABLE";
        
        // Get current time for created/modified (simplified)
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
        info.created_time = info.modified_time = ss.str();
        
        // Get row count if table storage is loaded
        auto storage_it = table_storage_.find(name);
        if (storage_it != table_storage_.end()) {
            info.row_count = storage_it->second->getRowCount();
        }
        
        tables_info.push_back(std::move(info));
    }
    
    return tables_info;
}

void Catalog::vacuum() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    for (const auto& [name, storage] : table_storage_) {
        storage->vacuum();
    }
}

void Catalog::analyze() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    for (const auto& [name, storage] : table_storage_) {
        storage->analyze();
    }
}

// CatalogManager implementation
std::unique_ptr<Catalog> CatalogManager::instance_;
std::mutex CatalogManager::instance_mutex_;

Catalog& CatalogManager::getInstance(const std::string& data_path) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!instance_) {
        instance_ = std::make_unique<Catalog>(data_path);
    }
    
    return *instance_;
}

void CatalogManager::shutdown() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

} // namespace snowflake