#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

// Simple test framework
class TestFramework {
    struct TestCase {
        std::string name;
        std::function<bool()> test_func;
    };

    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;

public:
    void add_test(const std::string& name, std::function<bool()> func) {
        tests.push_back({name, func});
    }

    bool run_all() {
        std::cout << "=== Running " << tests.size() << " tests ===" << std::endl;
        std::cout << std::endl;

        for (const auto& test : tests) {
            std::cout << "Running: " << test.name << "..." << std::endl;
            try {
                if (test.test_func()) {
                    std::cout << "✅ PASSED: " << test.name << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ FAILED: " << test.name << std::endl;
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "❌ EXCEPTION in " << test.name << ": " << e.what() << std::endl;
                failed++;
            }
            std::cout << std::endl;
        }

        std::cout << "================================" << std::endl;
        std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
        std::cout << "Success rate: " << (passed * 100 / tests.size()) << "%" << std::endl;

        if (failed == 0) {
            std::cout << "✅ ALL TESTS PASSED!" << std::endl;
            return true;
        } else {
            std::cout << "❌ SOME TESTS FAILED!" << std::endl;
            return false;
        }
    }

    int get_passed() const { return passed; }
    int get_failed() const { return failed; }
};

// Helper macros
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "  Assertion failed: " << #condition << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  Assertion failed: " << #a << " == " << #b << std::endl; \
            std::cerr << "    Expected: " << (b) << std::endl; \
            std::cerr << "    Got: " << (a) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "  Assertion failed: " << #a << " != " << #b << std::endl; \
            return false; \
        } \
    } while(0)

#endif // TEST_FRAMEWORK_H
