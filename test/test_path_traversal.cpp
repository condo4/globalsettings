#include <iostream>
#include <cassert>
#include "src/globalsettings.h"

// Test function to check if setId rejects malicious inputs
void testPathTraversal() {
    GlobalSettings settings;

    std::cout << "=== Path Traversal Protection Test ===" << std::endl;
    std::cout << "Initial ID: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 1: Attempt parent directory traversal
    std::cout << "Test 1: Attempting setId(\"../../etc/passwd\")..." << std::endl;
    std::string original_id = settings.id();
    settings.setId("../../etc/passwd");
    std::string after_attack = settings.id();
    assert(after_attack == original_id);
    std::cout << "✅ BLOCKED: ID remains: " << after_attack << std::endl;
    std::cout << std::endl;

    // Test 2: Attempt absolute path
    std::cout << "Test 2: Attempting setId(\"/etc/malicious\")..." << std::endl;
    settings.setId("/etc/malicious");
    assert(settings.id() == original_id);
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 3: Attempt Windows-style path
    std::cout << "Test 3: Attempting setId(\"..\\\\..\\\\windows\\\\system32\")..." << std::endl;
    settings.setId("..\\..\\windows\\system32");
    assert(settings.id() == original_id);
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 4: Attempt special characters
    std::cout << "Test 4: Attempting setId(\"malicious;rm -rf /\")..." << std::endl;
    settings.setId("malicious;rm -rf /");
    assert(settings.id() == original_id);
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 5: Attempt null bytes (encoded as string)
    std::cout << "Test 5: Attempting setId with special chars..." << std::endl;
    settings.setId("test@#$%");
    assert(settings.id() == original_id);
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 6: Valid IDs should work
    std::cout << "Test 6: Attempting setId(\"valid_app_123\")..." << std::endl;
    settings.setId("valid_app_123");
    assert(settings.id() == "valid_app_123");
    std::cout << "✅ ALLOWED: ID changed to: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 7: Valid with dashes
    std::cout << "Test 7: Attempting setId(\"my-app-v2\")..." << std::endl;
    settings.setId("my-app-v2");
    assert(settings.id() == "my-app-v2");
    std::cout << "✅ ALLOWED: ID changed to: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 8: Empty ID should be rejected
    std::cout << "Test 8: Attempting setId(\"\")..." << std::endl;
    settings.setId("");
    assert(settings.id() == "my-app-v2");  // Should remain unchanged
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    // Test 9: Hidden path traversal
    std::cout << "Test 9: Attempting setId(\"foo/../bar\")..." << std::endl;
    settings.setId("foo/../bar");
    assert(settings.id() == "my-app-v2");
    std::cout << "✅ BLOCKED: ID remains: " << settings.id() << std::endl;
    std::cout << std::endl;

    std::cout << "==================================" << std::endl;
    std::cout << "✅ ALL TESTS PASSED!" << std::endl;
    std::cout << "Path traversal attacks are properly blocked!" << std::endl;
}

int main() {
    try {
        testPathTraversal();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
