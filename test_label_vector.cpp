#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "include/label_vector.hpp"

using namespace surf;

int main() {
    std::cout << "=== LabelVector simdSearch Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Create test data for LabelVector
    std::cout << "1. Creating test data for LabelVector..." << std::endl;
    
    // Create multi-level label data
    std::vector<std::vector<label_t> > labels_per_level;
    
    // Level 0: Small array (should use linearSearch)
    std::vector<label_t> level0;
    level0.push_back('a');
    level0.push_back('b');
    labels_per_level.push_back(level0);
    
    // Level 1: Medium array (should use binarySearch)
    std::vector<label_t> level1;
    for (char c = 'a'; c <= 'k'; c++) {  // 11 elements
        level1.push_back(c);
    }
    labels_per_level.push_back(level1);
    
    // Level 2: Large array (should use simdSearch) - 20 elements
    std::vector<label_t> level2;
    for (char c = 'a'; c <= 't'; c++) {  // 20 elements
        level2.push_back(c);
    }
    labels_per_level.push_back(level2);
    
    // Level 3: Very large array (should definitely use simdSearch) - 30 elements
    std::vector<label_t> level3;
    for (int i = 0; i < 30; i++) {
        level3.push_back('A' + (i % 26));  // Mix of uppercase letters
    }
    labels_per_level.push_back(level3);
    
    std::cout << "   Created " << labels_per_level.size() << " levels of test data" << std::endl;
    std::cout << "   Level 0: " << level0.size() << " elements (linearSearch)" << std::endl;
    std::cout << "   Level 1: " << level1.size() << " elements (binarySearch)" << std::endl;
    std::cout << "   Level 2: " << level2.size() << " elements (simdSearch)" << std::endl;
    std::cout << "   Level 3: " << level3.size() << " elements (simdSearch)" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Create LabelVector instances
    std::cout << "2. Creating LabelVector instances..." << std::endl;
    
    // Full LabelVector with all levels
    LabelVector full_lv(labels_per_level);
    std::cout << "   Full LabelVector created with " << full_lv.getNumBytes() << " bytes" << std::endl;
    
    // LabelVector with only large levels (to force simdSearch usage)
    std::vector<std::vector<label_t> > large_levels;
    large_levels.push_back(level2);
    large_levels.push_back(level3);
    LabelVector large_lv(large_levels);
    std::cout << "   Large LabelVector created with " << large_lv.getNumBytes() << " bytes" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Test search functionality that will trigger simdSearch
    std::cout << "3. Testing search functionality (including simdSearch)..." << std::endl;
    
    // Test searches in the large level (level 2 data)
    position_t pos = level0.size() + level1.size();  // Start position for level 2
    position_t search_len = level2.size();
    
    std::cout << "   Testing searches in level 2 (20 elements, should use simdSearch):" << std::endl;
    
    // Test existing elements
    std::vector<char> test_chars = {'a', 'e', 'j', 'o', 't'};  // Various positions
    for (size_t i = 0; i < test_chars.size(); i++) {
        char target = test_chars[i];
        position_t test_pos = pos;
        bool found = full_lv.search(target, test_pos, search_len);
        std::cout << "     Search for '" << target << "': " << (found ? "FOUND" : "NOT FOUND");
        if (found) {
            std::cout << " at position " << test_pos << " (value: '" << (char)full_lv[test_pos] << "')";
        }
        std::cout << std::endl;
    }
    
    // Test non-existent elements
    std::vector<char> missing_chars = {'z', 'x', 'w'};
    for (size_t i = 0; i < missing_chars.size(); i++) {
        char target = missing_chars[i];
        position_t test_pos = pos;
        bool found = full_lv.search(target, test_pos, search_len);
        std::cout << "     Search for '" << target << "' (should not exist): " << (found ? "FOUND" : "NOT FOUND") << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: Test searches in very large level (level 3 data)
    std::cout << "4. Testing searches in level 3 (30 elements, should definitely use simdSearch)..." << std::endl;
    
    position_t pos3 = level0.size() + level1.size() + level2.size();  // Start position for level 3
    position_t search_len3 = level3.size();
    
    std::vector<char> test_chars3 = {'A', 'E', 'M', 'T', 'Z'};
    for (size_t i = 0; i < test_chars3.size(); i++) {
        char target = test_chars3[i];
        position_t test_pos = pos3;
        bool found = full_lv.search(target, test_pos, search_len3);
        std::cout << "     Search for '" << target << "': " << (found ? "FOUND" : "NOT FOUND");
        if (found) {
            std::cout << " at position " << test_pos << " (value: '" << (char)full_lv[test_pos] << "')";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Direct simdSearch testing with large_lv
    std::cout << "5. Direct testing of simdSearch with large arrays..." << std::endl;
    
    // Test the large_lv which starts directly with level2 data
    position_t direct_pos = 0;
    position_t direct_search_len = level2.size();
    
    std::cout << "   Testing direct simdSearch calls:" << std::endl;
    std::vector<char> direct_test_chars = {'a', 'c', 'g', 'n', 't'};
    for (size_t i = 0; i < direct_test_chars.size(); i++) {
        char target = direct_test_chars[i];
        position_t test_pos = direct_pos;
        bool found = large_lv.simdSearch(target, test_pos, direct_search_len);
        std::cout << "     simdSearch for '" << target << "': " << (found ? "FOUND" : "NOT FOUND");
        if (found) {
            std::cout << " at position " << test_pos << " (value: '" << (char)large_lv[test_pos] << "')";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Performance comparison hint
    std::cout << "6. Testing edge cases for simdSearch..." << std::endl;
    
    // Test search at boundaries
    position_t boundary_pos = 0;
    
    // Search for first element
    char first_target = level2[0];
    position_t first_pos = boundary_pos;
    bool found_first = large_lv.simdSearch(first_target, first_pos, level2.size());
    std::cout << "   Search for first element '" << first_target << "': " << (found_first ? "FOUND" : "NOT FOUND");
    if (found_first) {
        std::cout << " at position " << first_pos;
    }
    std::cout << std::endl;
    
    // Search for last element
    char last_target = level2[level2.size() - 1];
    position_t last_pos = boundary_pos;
    bool found_last = large_lv.simdSearch(last_target, last_pos, level2.size());
    std::cout << "   Search for last element '" << last_target << "': " << (found_last ? "FOUND" : "NOT FOUND");
    if (found_last) {
        std::cout << " at position " << last_pos;
    }
    std::cout << std::endl;
    
    // Test with exactly 16 elements (one SIMD chunk)
    position_t chunk_pos = 0;
    position_t chunk_len = 16;
    char mid_target = level2[8];  // Middle of first 16 elements
    position_t mid_pos = chunk_pos;
    bool found_mid = large_lv.simdSearch(mid_target, mid_pos, chunk_len);
    std::cout << "   Search in exact 16-element chunk for '" << mid_target << "': " << (found_mid ? "FOUND" : "NOT FOUND");
    if (found_mid) {
        std::cout << " at position " << mid_pos;
    }
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Test 7: Cleanup test
    std::cout << "7. Testing cleanup..." << std::endl;
    full_lv.destroy();
    large_lv.destroy();
    std::cout << "   LabelVector instances destroyed successfully" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== All LabelVector simdSearch tests completed! ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: The simdSearch function was exercised with:" << std::endl;
    std::cout << "  - Arrays of 20 and 30 elements (> 12 threshold)" << std::endl;
    std::cout << "  - Various search positions (first, middle, last)" << std::endl;
    std::cout << "  - Both existing and non-existent elements" << std::endl;
    std::cout << "  - Boundary conditions and exact chunk sizes" << std::endl;
    
    return 0;
}
