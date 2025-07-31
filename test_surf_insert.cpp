#include <iostream>
#include <vector>
#include <string>
#include "include/surf.hpp"

int main() {
    using namespace surf;
    
    std::cout << "=== SuRF Incremental Insert Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Create all four types of SuRF for incremental insertion
    std::cout << "1. Creating SuRF instances for incremental insertion..." << std::endl;
    
    // Basic SuRF
    SuRF surf_basic(true, 16, kNone, 0, 0);
    std::cout << "   Basic SuRF created successfully" << std::endl;
    
    // SuRF with hash suffix
    SuRF surf_hash(true, 16, kHash, 8, 0);
    std::cout << "   SuRF hash created successfully" << std::endl;
    
    // SuRF with real suffix
    SuRF surf_real(true, 16, kReal, 0, 8);
    std::cout << "   SuRF real created successfully" << std::endl;
    
    // SuRF with mixed suffix
    SuRF surf_mixed(true, 16, kMixed, 4, 4);
    std::cout << "   SuRF mixed created successfully" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Insert keys one by one in sorted order for all SuRF types
    std::vector<std::string> test_keys;
    test_keys.push_back("apple");
    test_keys.push_back("banana");
    test_keys.push_back("cherry");
    test_keys.push_back("date");
    test_keys.push_back("elderberry");
    
    std::cout << "2. Inserting keys incrementally into all SuRF types..." << std::endl;
    for (size_t i = 0; i < test_keys.size(); ++i) {
        const std::string& key = test_keys[i];
        
        bool success_basic = surf_basic.insert(key);
        bool success_hash = surf_hash.insert(key);
        bool success_real = surf_real.insert(key);
        bool success_mixed = surf_mixed.insert(key);
        
        std::cout << "   Insert '" << key << "':" << std::endl;
        std::cout << "     Basic SuRF: " << (success_basic ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "     Hash SuRF: " << (success_hash ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "     Real SuRF: " << (success_real ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "     Mixed SuRF: " << (success_mixed ? "SUCCESS" : "FAILED") << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: Check if keys were inserted in all SuRF types
    std::cout << "3. Checking if SuRF instances have keys..." << std::endl;
    bool has_keys_basic = surf_basic.hasKeys();
    bool has_keys_hash = surf_hash.hasKeys();
    bool has_keys_real = surf_real.hasKeys();
    bool has_keys_mixed = surf_mixed.hasKeys();
    
    std::cout << "   Basic SuRF has keys: " << (has_keys_basic ? "YES" : "NO") << std::endl;
    std::cout << "   Hash SuRF has keys: " << (has_keys_hash ? "YES" : "NO") << std::endl;
    std::cout << "   Real SuRF has keys: " << (has_keys_real ? "YES" : "NO") << std::endl;
    std::cout << "   Mixed SuRF has keys: " << (has_keys_mixed ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
    // Test 4: Try to lookup before finalization (should fail for all)
    std::cout << "4. Testing lookup before finalization (should fail)..." << std::endl;
    bool found_before_basic = surf_basic.lookupKey("apple");
    bool found_before_hash = surf_hash.lookupKey("apple");
    bool found_before_real = surf_real.lookupKey("apple");
    bool found_before_mixed = surf_mixed.lookupKey("apple");
    
    std::cout << "   Lookup 'apple' before finalization:" << std::endl;
    std::cout << "     Basic SuRF: " << (found_before_basic ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Hash SuRF: " << (found_before_hash ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Real SuRF: " << (found_before_real ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Mixed SuRF: " << (found_before_mixed ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << std::endl;
    
    // Test 5: Finalize all SuRF instances
    std::cout << "5. Finalizing all SuRF instances..." << std::endl;
    surf_basic.finalize();
    surf_hash.finalize();
    surf_real.finalize();
    surf_mixed.finalize();
    std::cout << "   All SuRF instances finalized successfully" << std::endl;
    std::cout << std::endl;
    
    // Test 6: Test lookups after finalization for all SuRF types
    std::cout << "6. Testing lookups after finalization:" << std::endl;
    for (size_t i = 0; i < test_keys.size(); ++i) {
        const std::string& key = test_keys[i];
        
        bool found_basic = surf_basic.lookupKey(key);
        bool found_hash = surf_hash.lookupKey(key);
        bool found_real = surf_real.lookupKey(key);
        bool found_mixed = surf_mixed.lookupKey(key);
        
        std::cout << "   Lookup '" << key << "':" << std::endl;
        std::cout << "     Basic SuRF: " << (found_basic ? "FOUND" : "NOT FOUND") << std::endl;
        std::cout << "     Hash SuRF: " << (found_hash ? "FOUND" : "NOT FOUND") << std::endl;
        std::cout << "     Real SuRF: " << (found_real ? "FOUND" : "NOT FOUND") << std::endl;
        std::cout << "     Mixed SuRF: " << (found_mixed ? "FOUND" : "NOT FOUND") << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: Test lookup of non-existent keys for all SuRF types
    std::cout << "7. Testing lookup of non-existent keys..." << std::endl;
    
    std::string non_existent_key = "nonexistent";
    bool found_ne_basic = surf_basic.lookupKey(non_existent_key);
    bool found_ne_hash = surf_hash.lookupKey(non_existent_key);
    bool found_ne_real = surf_real.lookupKey(non_existent_key);
    bool found_ne_mixed = surf_mixed.lookupKey(non_existent_key);
    
    std::cout << "   Lookup '" << non_existent_key << "':" << std::endl;
    std::cout << "     Basic SuRF: " << (found_ne_basic ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Hash SuRF: " << (found_ne_hash ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Real SuRF: " << (found_ne_real ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Mixed SuRF: " << (found_ne_mixed ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << std::endl;

    // Test 8: Test lookup of non-existent key similar to existing keys (potential false positive)
    std::cout << "8. Testing lookup of non-existent key similar to existing keys..." << std::endl;
    std::string fp_test_key = "banani";
    bool found_fp_basic = surf_basic.lookupKey(fp_test_key);
    bool found_fp_hash = surf_hash.lookupKey(fp_test_key);
    bool found_fp_real = surf_real.lookupKey(fp_test_key);
    bool found_fp_mixed = surf_mixed.lookupKey(fp_test_key);
    
    std::cout << "   Lookup '" << fp_test_key << "' (potential false positive):" << std::endl;
    std::cout << "     Basic SuRF: " << (found_fp_basic ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Hash SuRF: " << (found_fp_hash ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Real SuRF: " << (found_fp_real ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "     Mixed SuRF: " << (found_fp_mixed ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << std::endl;

    // Test 9: Test invalid insertion order with new SuRF instances
    std::cout << "9. Testing invalid insertion order with new SuRF instances..." << std::endl;
    SuRF test_order_basic(true, 16, kNone, 0, 0);
    SuRF test_order_hash(true, 16, kHash, 8, 0);
    
    bool success1_basic = test_order_basic.insert("zebra");
    bool success2_basic = test_order_basic.insert("apple"); // This should fail - out of order
    bool success1_hash = test_order_hash.insert("zebra");
    bool success2_hash = test_order_hash.insert("apple"); // This should fail - out of order
    
    std::cout << "   Basic SuRF:" << std::endl;
    std::cout << "     Insert 'zebra': " << (success1_basic ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "     Insert 'apple' (out of order): " << (success2_basic ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   Hash SuRF:" << std::endl;
    std::cout << "     Insert 'zebra': " << (success1_hash ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "     Insert 'apple' (out of order): " << (success2_hash ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << std::endl;
        

    std::cout << "=== All tests completed! ===" << std::endl;
    return 0;
}
