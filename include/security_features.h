#pragma once

#include "types.h"
#include "advanced_features.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>

namespace snowflake {

// Cryptographic functions for security
class CryptoManager {
public:
    enum class HashAlgorithm {
        SHA256,
        SHA512,
        BLAKE2B,
        ARGON2
    };
    
    enum class EncryptionAlgorithm {
        AES256_GCM,
        CHACHA20_POLY1305,
        AES256_CBC
    };
    
private:
    std::vector<uint8_t> master_key_;
    mutable std::shared_mutex mutex_;
    
public:
    explicit CryptoManager(const std::string& master_key_file = "");
    
    // Hashing
    std::vector<uint8_t> hash(const std::vector<uint8_t>& data, HashAlgorithm algo);
    std::string hash_string(const std::string& data, HashAlgorithm algo);
    bool verify_hash(const std::vector<uint8_t>& data, const std::vector<uint8_t>& hash, HashAlgorithm algo);
    
    // Encryption/Decryption
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, EncryptionAlgorithm algo);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext, EncryptionAlgorithm algo);
    
    // Key management
    void rotate_master_key(const std::string& new_key_file);
    std::vector<uint8_t> derive_key(const std::string& context, size_t key_size);
    
    // Column-level encryption
    std::vector<uint8_t> encrypt_column_data(const std::vector<Value>& values, const std::string& column_name);
    std::vector<Value> decrypt_column_data(const std::vector<uint8_t>& encrypted_data, DataType type, const std::string& column_name);
};

// Advanced authentication and authorization
class SecurityManager {
public:
    enum class Permission {
        READ,
        WRITE,
        CREATE_TABLE,
        DROP_TABLE,
        ALTER_TABLE,
        ADMIN
    };
    
    struct User {
        std::string username;
        std::string password_hash;
        std::vector<Permission> permissions;
        std::vector<std::string> allowed_tables;
        bool is_admin;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_login;
        bool enabled;
    };
    
    struct Session {
        std::string session_id;
        std::string username;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_accessed;
        std::chrono::seconds timeout;
        std::string client_ip;
    };
    
private:
    std::unordered_map<std::string, User> users_;
    std::unordered_map<std::string, Session> sessions_;
    CryptoManager& crypto_manager_;
    mutable std::shared_mutex mutex_;
    std::chrono::seconds default_session_timeout_;
    
public:
    explicit SecurityManager(CryptoManager& crypto, 
                           std::chrono::seconds session_timeout = std::chrono::seconds(3600));
    
    // User management
    void create_user(const std::string& username, const std::string& password, 
                    const std::vector<Permission>& permissions = {});
    void delete_user(const std::string& username);
    void grant_permission(const std::string& username, Permission permission);
    void revoke_permission(const std::string& username, Permission permission);
    void grant_table_access(const std::string& username, const std::string& table_name);
    void revoke_table_access(const std::string& username, const std::string& table_name);
    
    // Authentication
    std::string authenticate(const std::string& username, const std::string& password, const std::string& client_ip);
    bool validate_session(const std::string& session_id);
    void logout(const std::string& session_id);
    void cleanup_expired_sessions();
    
    // Authorization
    bool has_permission(const std::string& session_id, Permission permission);
    bool can_access_table(const std::string& session_id, const std::string& table_name);
    std::string get_username_from_session(const std::string& session_id);
    
    // Auditing
    struct AuditLog {
        std::chrono::steady_clock::time_point timestamp;
        std::string username;
        std::string action;
        std::string table_name;
        std::string client_ip;
        bool success;
    };
    
    void log_action(const std::string& username, const std::string& action, 
                   const std::string& table_name, const std::string& client_ip, bool success);
    std::vector<AuditLog> get_audit_logs(std::chrono::steady_clock::time_point start,
                                        std::chrono::steady_clock::time_point end) const;
};

// Backup and recovery system
class BackupManager {
public:
    enum class BackupType {
        FULL,
        INCREMENTAL,
        DIFFERENTIAL
    };
    
    struct BackupMetadata {
        std::string backup_id;
        BackupType type;
        std::chrono::steady_clock::time_point timestamp;
        std::vector<std::string> tables_included;
        size_t size_bytes;
        std::string checksum;
        std::string storage_location;
    };
    
private:
    std::string backup_directory_;
    class Catalog& catalog_;
    CryptoManager& crypto_manager_;
    std::vector<BackupMetadata> backup_history_;
    mutable std::shared_mutex mutex_;
    
public:
    explicit BackupManager(const std::string& backup_dir, class Catalog& catalog, CryptoManager& crypto);
    
    // Backup operations
    std::string create_backup(BackupType type, const std::vector<std::string>& tables = {});
    void schedule_backup(BackupType type, std::chrono::seconds interval, const std::vector<std::string>& tables = {});
    
    // Recovery operations
    void restore_backup(const std::string& backup_id, const std::vector<std::string>& tables = {});
    std::vector<BackupMetadata> list_backups() const;
    void delete_backup(const std::string& backup_id);
    
    // Point-in-time recovery
    void enable_point_in_time_recovery();
    void restore_to_point_in_time(std::chrono::steady_clock::time_point target_time);
    
    // Validation
    bool verify_backup(const std::string& backup_id);
    void cleanup_old_backups(std::chrono::hours retention_period);
};

// Advanced query planner with machine learning
class IntelligentPlanner {
public:
    struct ExecutionNode {
        std::string operation_type;
        std::unordered_map<std::string, std::string> properties;
        std::vector<std::unique_ptr<ExecutionNode>> children;
        double estimated_cost;
        size_t estimated_rows;
        std::chrono::microseconds estimated_time;
    };
    
    struct PlanAlternative {
        std::unique_ptr<ExecutionNode> root;
        double total_cost;
        std::vector<std::string> assumptions;
        double confidence_score;
    };
    
private:
    class Catalog& catalog_;
    MLQueryOptimizer& ml_optimizer_;
    
    // Cost models
    double calculate_scan_cost(const std::string& table_name, const std::vector<std::string>& columns);
    double calculate_filter_cost(size_t input_rows, double selectivity);
    double calculate_join_cost(size_t left_rows, size_t right_rows, const std::string& join_type);
    double calculate_sort_cost(size_t rows, size_t columns);
    double calculate_aggregation_cost(size_t rows, size_t groups);
    
public:
    explicit IntelligentPlanner(class Catalog& catalog, MLQueryOptimizer& ml_opt);
    
    std::vector<PlanAlternative> generate_plans(const std::string& sql);
    PlanAlternative select_best_plan(const std::vector<PlanAlternative>& alternatives);
    
    // Advanced optimizations
    std::unique_ptr<ExecutionNode> apply_predicate_pushdown(std::unique_ptr<ExecutionNode> plan);
    std::unique_ptr<ExecutionNode> apply_join_reordering(std::unique_ptr<ExecutionNode> plan);
    std::unique_ptr<ExecutionNode> apply_projection_pushdown(std::unique_ptr<ExecutionNode> plan);
    std::unique_ptr<ExecutionNode> apply_common_subexpression_elimination(std::unique_ptr<ExecutionNode> plan);
    
    // Plan visualization
    std::string serialize_plan_json(const ExecutionNode& plan);
    std::string generate_plan_graphviz(const ExecutionNode& plan);
};

// Real-time monitoring and alerting
class MonitoringSystem {
public:
    enum class AlertSeverity {
        INFO,
        WARNING,
        CRITICAL
    };
    
    struct Alert {
        std::string id;
        std::string name;
        std::string description;
        AlertSeverity severity;
        std::chrono::steady_clock::time_point timestamp;
        std::unordered_map<std::string, std::string> labels;
        bool resolved;
    };
    
    struct Threshold {
        std::string metric_name;
        std::string condition; // ">" | "<" | "==" | "!="
        double value;
        std::chrono::seconds duration;
        AlertSeverity severity;
        std::string alert_message;
    };
    
private:
    MetricsCollector& metrics_collector_;
    TimeSeriesAnalyzer& time_series_;
    std::vector<Threshold> thresholds_;
    std::vector<Alert> active_alerts_;
    std::vector<std::function<void(const Alert&)>> alert_handlers_;
    std::thread monitoring_thread_;
    std::atomic<bool> running_;
    mutable std::shared_mutex mutex_;
    
public:
    explicit MonitoringSystem(MetricsCollector& metrics, TimeSeriesAnalyzer& ts);
    ~MonitoringSystem();
    
    void start();
    void stop();
    
    // Threshold management
    void add_threshold(const Threshold& threshold);
    void remove_threshold(const std::string& metric_name);
    void update_threshold(const std::string& metric_name, const Threshold& new_threshold);
    
    // Alert management
    std::vector<Alert> get_active_alerts() const;
    std::vector<Alert> get_alert_history(std::chrono::steady_clock::time_point start,
                                        std::chrono::steady_clock::time_point end) const;
    void resolve_alert(const std::string& alert_id);
    
    // Alert handlers
    void add_alert_handler(std::function<void(const Alert&)> handler);
    void send_email_alert(const std::string& smtp_server, const std::string& to_email);
    void send_slack_alert(const std::string& webhook_url);
    void send_pagerduty_alert(const std::string& integration_key);
    
private:
    void monitoring_worker();
    void check_thresholds();
    void trigger_alert(const Threshold& threshold, double current_value);
};

// Advanced indexing system
class IndexManager {
public:
    enum class IndexType {
        BTREE,
        HASH,
        BITMAP,
        BLOOM_FILTER,
        INVERTED,
        SPATIAL,
        FULL_TEXT
    };
    
    struct IndexMetadata {
        std::string name;
        std::string table_name;
        std::vector<std::string> columns;
        IndexType type;
        size_t size_bytes;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_used;
        size_t usage_count;
        double selectivity_improvement;
    };
    
private:
    std::unordered_map<std::string, IndexMetadata> indexes_;
    std::string index_directory_;
    mutable std::shared_mutex mutex_;
    
public:
    explicit IndexManager(const std::string& index_dir);
    
    // Index creation
    void create_index(const std::string& name, const std::string& table_name,
                     const std::vector<std::string>& columns, IndexType type);
    void drop_index(const std::string& name);
    
    // Index usage
    bool can_use_index(const std::string& table_name, const std::vector<std::string>& columns,
                      const std::string& operation);
    std::vector<std::string> suggest_indexes(const std::string& sql);
    
    // Maintenance
    void rebuild_index(const std::string& name);
    void analyze_index_usage();
    std::vector<std::string> get_unused_indexes(std::chrono::hours threshold) const;
    
    // Statistics
    std::vector<IndexMetadata> list_indexes() const;
    IndexMetadata get_index_stats(const std::string& name) const;
    size_t total_index_size() const;
};

} // namespace snowflake