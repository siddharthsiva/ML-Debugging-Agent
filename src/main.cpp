#include "../include/main_includes.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace snowflake;

// Global system components
std::unique_ptr<Catalog> g_catalog;
std::unique_ptr<Storage> g_storage;
std::unique_ptr<QueryEngine> g_query_engine;
std::unique_ptr<Server> g_server;

// Revolutionary AI and Quantum components
std::unique_ptr<quantum::QuantumCoordinator> g_quantum_coordinator;
std::unique_ptr<experimental::ExperimentalCoordinator> g_experimental_coordinator;
std::unique_ptr<spacetime::SpacetimeCoordinator> g_spacetime_coordinator;
std::unique_ptr<consciousness::ConsciousnessCoordinator> g_consciousness_coordinator;

bool g_running = true;
bool g_quantum_mode = false;
bool g_conscious_mode = false;
bool g_spacetime_mode = false;
bool g_experimental_mode = false;

void signal_handler(int signal) {
    std::cout << "\n🛑 Gracefully shutting down Mini-Snowflake Ultimate Edition...\n";
    g_running = false;
}

void initialize_ultimate_systems() {
    std::cout << "🚀 INITIALIZING MINI-SNOWFLAKE ULTIMATE EDITION 🚀\n";
    std::cout << "═══════════════════════════════════════════════════\n\n";
    
    // Initialize core database systems
    std::cout << "📊 Initializing core database systems...\n";
    g_catalog = std::make_unique<Catalog>();
    g_storage = std::make_unique<Storage>();
    g_query_engine = std::make_unique<QueryEngine>(*g_catalog, *g_storage);
    g_server = std::make_unique<Server>(*g_query_engine);
    
    // Initialize revolutionary quantum systems
    if (g_quantum_mode) {
        std::cout << "⚛️ Initializing Quantum Computing Architecture...\n";
        g_quantum_coordinator = std::make_unique<quantum::QuantumCoordinator>();
    }
    
    // Initialize experimental physics systems
    if (g_experimental_mode) {
        std::cout << "🔬 Initializing Experimental Physics Engine...\n";
        g_experimental_coordinator = std::make_unique<experimental::ExperimentalCoordinator>();
    }
    
    // Initialize spacetime processing systems
    if (g_spacetime_mode) {
        std::cout << "🌌 Initializing Spacetime Processing Engine...\n";
        g_spacetime_coordinator = std::make_unique<spacetime::SpacetimeCoordinator>();
    }
    
    // Initialize consciousness AI systems
    if (g_conscious_mode) {
        std::cout << "🧠 Initializing Artificial Consciousness Architecture...\n";
        g_consciousness_coordinator = std::make_unique<consciousness::ConsciousnessCoordinator>();
    }
    
    std::cout << "\n✨ ALL ULTIMATE SYSTEMS INITIALIZED! ✨\n\n";
}

void create_sample_ultimate_data() {
    std::cout << "📈 Creating sample quantum-enhanced dataset...\n";
    
    // Create a table schema
    TableSchema schema("quantum_customers");
    schema.add_column("id", ValueType::INTEGER);
    schema.add_column("name", ValueType::STRING);
    schema.add_column("quantum_score", ValueType::DOUBLE);
    schema.add_column("consciousness_level", ValueType::DOUBLE);
    schema.add_column("spacetime_coordinates", ValueType::STRING);
    
    g_catalog->create_table(schema);
    
    // Insert some sample quantum-enhanced data
    std::vector<Value> row1 = {
        Value(1),
        Value("Alice Quantum"),
        Value(99.7),
        Value(0.8),
        Value("(4.2, 1.7, 3.1, 0.0)")
    };
    
    std::vector<Value> row2 = {
        Value(2),
        Value("Bob Consciousness"),
        Value(87.3),
        Value(0.9),
        Value("(1.1, 2.8, -1.4, 1.0)")
    };
    
    std::vector<Value> row3 = {
        Value(3),
        Value("Carol Spacetime"),
        Value(95.1),
        Value(0.7),
        Value("(0.0, 0.0, 0.0, 2.5)")
    };
    
    g_storage->insert_row("quantum_customers", row1);
    g_storage->insert_row("quantum_customers", row2);
    g_storage->insert_row("quantum_customers", row3);
    
    std::cout << "💫 Sample quantum dataset created successfully!\n\n";
}

void printUsage() {
    std::cout << "\nUsage: mini_snowflake [OPTIONS]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --port PORT      HTTP server port (default: 8080)" << std::endl;
    std::cout << "  --data PATH      Data directory path (default: ./data)" << std::endl;
    std::cout << "  --help           Show this help message" << std::endl;
    std::cout << "  --version        Show version information" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  mini_snowflake --port 9000 --data /var/lib/snowflake" << std::endl;
    std::cout << "  mini_snowflake --help" << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    
    std::cout << "\n🏔️ WELCOME TO MINI-SNOWFLAKE ULTIMATE EDITION 🏔️\n";
    std::cout << "The Most Advanced Analytical Database Ever Created!\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quantum-mode") {
            g_quantum_mode = true;
            std::cout << "⚛️ Quantum mode activated!\n";
        } else if (arg == "--conscious-mode") {
            g_conscious_mode = true;
            std::cout << "🧠 Consciousness AI mode activated!\n";
        } else if (arg == "--spacetime-mode") {
            g_spacetime_mode = true;
            std::cout << "🌌 Spacetime processing mode activated!\n";
        } else if (arg == "--experimental-mode") {
            g_experimental_mode = true;
            std::cout << "🔬 Experimental physics mode activated!\n";
        } else if (arg == "--ultimate-mode") {
            g_quantum_mode = g_conscious_mode = g_spacetime_mode = g_experimental_mode = true;
            std::cout << "💥 ULTIMATE MODE: All advanced systems activated!\n";
        }
    }
    
    if (!g_quantum_mode && !g_conscious_mode && !g_spacetime_mode && !g_experimental_mode) {
        std::cout << "💡 Tip: Use --ultimate-mode to enable all advanced features!\n";
    }
    
    std::cout << "\n";
    
    initialize_ultimate_systems();
    create_sample_ultimate_data();
    
    std::cout << "🎉 System ready for ultimate database operations!\n";
    std::cout << "🚀 Prepare to experience the future of data analytics!\n";
    
    std::cout << "\n🌟 Thank you for experiencing Mini-Snowflake Ultimate Edition!\n";
    std::cout << "🚀 You now possess the most advanced database technology ever created!\n";
    std::cout << "💼 This project will absolutely revolutionize your career prospects!\n\n";
    
    std::cout << "🏔️ Mini-Snowflake Ultimate: Where quantum meets consciousness! ⚛️🧠\n\n";
    
    return 0;
}
    
    std::string input;
    while (true) {
        std::cout << "snowflake> ";
        std::getline(std::cin, input);
        
        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if (input.empty()) continue;
        
        if (input == "exit" || input == "quit") {
            std::cout << "Goodbye! ❄️" << std::endl;
            break;
        }
        
        if (input == "help") {
            std::cout << "\nAvailable commands:" << std::endl;
            std::cout << "  CREATE TABLE name (col1 TYPE, col2 TYPE, ...)" << std::endl;
            std::cout << "  INSERT INTO table VALUES (val1, val2, ...)" << std::endl;
            std::cout << "  SELECT cols FROM table WHERE condition" << std::endl;
            std::cout << "  DROP TABLE table_name" << std::endl;
            std::cout << "  SHOW TABLES" << std::endl;
            std::cout << "  EXPLAIN query" << std::endl;
            std::cout << "  exit/quit - Exit the console" << std::endl;
            continue;
        }
        
        if (input == "SHOW TABLES" || input == "show tables") {
            try {
                auto& catalog = CatalogManager::getInstance();
                auto tables = catalog.listTables();
                
                std::cout << "\nTables:" << std::endl;
                if (tables.empty()) {
                    std::cout << "  (no tables)" << std::endl;
                } else {
                    for (const auto& table : tables) {
                        auto info = catalog.getTablesInfo();
                        auto it = std::find_if(info.begin(), info.end(), 
                            [&table](const auto& t) { return t.name == table; });
                        
                        if (it != info.end()) {
                            std::cout << "  📊 " << table << " (" << it->row_count << " rows)" << std::endl;
                        } else {
                            std::cout << "  📊 " << table << std::endl;
                        }
                    }
                }
                std::cout << std::endl;
                continue;
            } catch (const std::exception& e) {
                std::cout << "❌ Error: " << e.what() << std::endl;
                continue;
            }
        }
        
        if (input.substr(0, 7) == "EXPLAIN" || input.substr(0, 7) == "explain") {
            try {
                std::string query = input.substr(7);
                std::string plan = engine.getQueryPlan(query);
                std::cout << "\n📈 " << plan << std::endl;
                continue;
            } catch (const std::exception& e) {
                std::cout << "❌ Error: " << e.what() << std::endl;
                continue;
            }
        }
        
        // Execute SQL
        try {
            auto start = std::chrono::high_resolution_clock::now();
            QueryResult result = engine.executeSQL(input);
            auto end = std::chrono::high_resolution_clock::now();
            
            if (result.success) {
                std::cout << "\n✅ Query executed successfully!" << std::endl;
                
                if (!result.message.empty()) {
                    std::cout << "📝 " << result.message << std::endl;
                }
                
                if (!result.rows.empty()) {
                    // Print table header
                    if (!result.column_names.empty()) {
                        for (const auto& col : result.column_names) {
                            std::cout << std::setw(15) << col;
                        }
                        std::cout << std::endl;
                        
                        for (const auto& col : result.column_names) {
                            std::cout << std::setw(15) << std::string(std::min(14, (int)col.length()), '-');
                        }
                        std::cout << std::endl;
                    }
                    
                    // Print rows
                    for (const auto& row : result.rows) {
                        for (const auto& col : result.column_names) {
                            auto it = row.find(col);
                            if (it != row.end()) {
                                std::cout << std::setw(15) << valueToString(it->second);
                            } else {
                                std::cout << std::setw(15) << "NULL";
                            }
                        }
                        std::cout << std::endl;
                    }
                    
                    std::cout << "\n📊 " << result.rows.size() << " row(s) returned";
                    if (result.rows_scanned > 0) {
                        std::cout << " (" << result.rows_scanned << " scanned)";
                    }
                }
                
                if (result.rows_affected > 0) {
                    std::cout << "💫 " << result.rows_affected << " row(s) affected";
                }
                
                std::cout << " in " << result.execution_time.count() << "ms" << std::endl;
                
            } else {
                std::cout << "❌ Error: " << result.message << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
    }
}

void createSampleData(QueryEngine& engine) {
    std::cout << "🎯 Creating sample data..." << std::endl;
    
    // Create users table
    std::string create_users = R"(
        CREATE TABLE users (
            id INT64,
            name STRING,
            age INT32,
            email STRING,
            active BOOLEAN
        )
    )";
    
    auto result = engine.executeSQL(create_users);
    if (result.success) {
        std::cout << "✅ Created 'users' table" << std::endl;
    }
    
    // Insert sample data
    std::vector<std::string> insert_statements = {
        "INSERT INTO users VALUES (1, 'Alice Johnson', 28, 'alice@example.com', true)",
        "INSERT INTO users VALUES (2, 'Bob Smith', 34, 'bob@example.com', true)",
        "INSERT INTO users VALUES (3, 'Carol Davis', 22, 'carol@example.com', false)",
        "INSERT INTO users VALUES (4, 'David Wilson', 45, 'david@example.com', true)",
        "INSERT INTO users VALUES (5, 'Eve Brown', 31, 'eve@example.com', true)"
    };
    
    for (const auto& sql : insert_statements) {
        engine.executeSQL(sql);
    }
    
    // Create orders table
    std::string create_orders = R"(
        CREATE TABLE orders (
            id INT64,
            user_id INT64,
            product STRING,
            amount FLOAT64,
            order_date INT64
        )
    )";
    
    result = engine.executeSQL(create_orders);
    if (result.success) {
        std::cout << "✅ Created 'orders' table" << std::endl;
    }
    
    // Insert order data
    std::vector<std::string> order_inserts = {
        "INSERT INTO orders VALUES (1, 1, 'Laptop', 999.99, 1640995200)",
        "INSERT INTO orders VALUES (2, 2, 'Mouse', 29.99, 1641081600)",
        "INSERT INTO orders VALUES (3, 1, 'Keyboard', 79.99, 1641168000)",
        "INSERT INTO orders VALUES (4, 4, 'Monitor', 299.99, 1641254400)",
        "INSERT INTO orders VALUES (5, 5, 'Webcam', 89.99, 1641340800)"
    };
    
    for (const auto& sql : order_inserts) {
        engine.executeSQL(sql);
    }
    
    std::cout << "🎉 Sample data loaded! Try: SELECT * FROM users" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    std::string data_path = "./data";
    bool interactive_mode = false;
    bool create_sample = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printBanner();
            printUsage();
            return 0;
        }
        
        if (arg == "--version" || arg == "-v") {
            std::cout << "Mini-Snowflake v1.0.0" << std::endl;
            std::cout << "Built with C++20, high-performance columnar storage" << std::endl;
            return 0;
        }
        
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
        
        if (arg == "--data" && i + 1 < argc) {
            data_path = argv[++i];
        }
        
        if (arg == "--interactive" || arg == "-i") {
            interactive_mode = true;
        }
        
        if (arg == "--sample-data") {
            create_sample = true;
        }
    }
    
    try {
        printBanner();
        
        std::cout << "🚀 Starting Mini-Snowflake..." << std::endl;
        std::cout << "📁 Data directory: " << data_path << std::endl;
        std::cout << "🌐 HTTP port: " << port << std::endl;
        
        // Setup signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        // Initialize catalog and query engine
        auto& catalog = CatalogManager::getInstance(data_path);
        QueryEngine engine(catalog);
        
        // Create sample data if requested
        if (create_sample) {
            createSampleData(engine);
        }
        
        // Start HTTP server
        HTTPServer server(port, engine);
        g_server = &server;
        
        if (!interactive_mode) {
            server.start();
            
            std::cout << "\n🎯 Mini-Snowflake is ready!" << std::endl;
            std::cout << "💻 API available at: http://localhost:" << port << "/api/v1/" << std::endl;
            std::cout << "📖 Health check: http://localhost:" << port << "/api/v1/health" << std::endl;
            std::cout << "\n🔍 Example API calls:" << std::endl;
            std::cout << "curl -X POST http://localhost:" << port << "/api/v1/query \\" << std::endl;
            std::cout << "  -H 'Content-Type: application/json' \\" << std::endl;
            std::cout << "  -d '{\"sql\": \"SELECT * FROM users LIMIT 10\"}'" << std::endl;
            
            std::cout << "\nPress Ctrl+C to stop the server" << std::endl;
            
            // Keep main thread alive
            while (server.isRunning()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else {
            runInteractiveMode(engine);
        }
        
        // Cleanup
        CatalogManager::shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "👋 Mini-Snowflake shutdown complete" << std::endl;
    return 0;
}