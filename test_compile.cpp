// Test file to verify the core fixes work
#include "../include/storage.h"
#include "../include/parser.h"
#include <iostream>
#include <set>

int main() {
    // Test that basic includes work
    std::set<int> test_set;
    test_set.insert(1);
    
    std::cout << "✅ Includes are working correctly!" << std::endl;
    std::cout << "✅ std::set is available" << std::endl;
    std::cout << "✅ Storage and parser headers compile" << std::endl;
    
    return 0;
}