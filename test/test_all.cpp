#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"
#include <thread>
#include <vector>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <unistd.h>

// Include all test functions here

// ============ BASIC FUNCTIONALITY TESTS ============

bool test_get_default() {
    GlobalSettings settings;
    auto val = settings.get("nonexistent_key", "", SettingValue("default_value"));
    ASSERT_EQ(valueToStr(val), "default_value");
    return true;
}

bool test_get_different_types() {
    GlobalSettings settings;
    auto val1 = settings.get("key1", "", SettingValue("string"));
    ASSERT_EQ(valueToStr(val1), "string");

    auto val2 = settings.get("key2", "", SettingValue(42));
    ASSERT_EQ(valueToInt(val2), 42);

    auto val4 = settings.get("key4", "", SettingValue(true));
    ASSERT_EQ(valueToStr(val4), "true");

    return true;
}

bool test_id_method() {
    GlobalSettings settings;
    std::string id = settings.id();
    ASSERT(!id.empty());
    ASSERT(id.find("/") == std::string::npos);
    ASSERT(id.find("\\") == std::string::npos);
    ASSERT(id.find("..") == std::string::npos);
    return true;
}

bool test_setid_valid() {
    GlobalSettings settings;
    settings.setId("test_app");
    ASSERT_EQ(settings.id(), "test_app");
    settings.setId("another-app_123");
    ASSERT_EQ(settings.id(), "another-app_123");
    return true;
}

bool test_size_method() {
    GlobalSettings settings;
    size_t size = settings.size();
    // size_t is always >= 0, just verify it exists
    (void)size;
    return true;
}

bool test_foreach_method() {
    GlobalSettings settings;
    int count = 0;
    bool success = true;
    settings.forEach([&count, &success](const SettingKey& key, const SettingValue& /*value*/) {
        count++;
        if (key.second.empty()) {
            success = false;
        }
    });
    ASSERT(success);
    ASSERT(count >= 0);
    return true;
}

bool test_multiple_instances() {
    GlobalSettings settings1;
    GlobalSettings settings2;
    ASSERT(!settings1.id().empty());
    ASSERT(!settings2.id().empty());
    settings1.setId("app1");
    settings2.setId("app2");
    ASSERT_EQ(settings1.id(), "app1");
    ASSERT_EQ(settings2.id(), "app2");
    return true;
}

// ============ INPUT VALIDATION TESTS ============

bool test_path_traversal_protection() {
    GlobalSettings settings;
    std::string original_id = settings.id();

    settings.setId("../../etc/passwd");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("/etc/malicious");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("..\\..\\windows\\system32");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("malicious;rm -rf /");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("test@#$%");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("");
    ASSERT_EQ(settings.id(), original_id);

    settings.setId("foo/../bar");
    ASSERT_EQ(settings.id(), original_id);

    return true;
}

bool test_malformed_keys() {
    GlobalSettings settings;

    auto val1 = settings.get("[category", "", SettingValue("default"));
    ASSERT_EQ(valueToStr(val1), "default");

    auto val2 = settings.get("[category]key", "", SettingValue("test"));
    ASSERT_EQ(valueToStr(val2), "test");

    auto val3 = settings.get("", "", SettingValue("empty"));
    ASSERT_EQ(valueToStr(val3), "empty");

    return true;
}

bool test_value_conversions() {
    SettingValue str_val("123");
    ASSERT_EQ(valueToInt(str_val), 123);

    SettingValue invalid_str("not_a_number");
    ASSERT_EQ(valueToInt(invalid_str), 0);

    SettingValue int_val(42);
    ASSERT_EQ(valueToStr(int_val), "42");

    SettingValue bool_val(true);
    ASSERT_EQ(valueToStr(bool_val), "true");

    return true;
}

bool test_make_value() {
    auto val1 = makeValue("\"hello world\"");
    ASSERT_EQ(valueToStr(val1), "hello world");

    auto val2 = makeValue("'test'");
    ASSERT_EQ(valueToStr(val2), "test");

    auto val3 = makeValue("42");
    ASSERT_EQ(valueToInt(val3), 42);

    auto val5 = makeValue("true");
    ASSERT_EQ(valueToStr(val5), "true");

    auto val6 = makeValue("false");
    ASSERT_EQ(valueToStr(val6), "false");

    auto val7 = makeValue("on");
    ASSERT_EQ(valueToStr(val7), "true");

    auto val8 = makeValue("off");
    ASSERT_EQ(valueToStr(val8), "false");

    return true;
}

// ============ THREAD SAFETY TESTS ============

bool test_concurrent_reads() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 10;
    const int ITERATIONS = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; i++) {
                try {
                    auto val = settings.get("test_key", "", SettingValue("default"));
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(errors.load(), 0);
    return true;
}

bool test_concurrent_id_reads() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 10;
    const int ITERATIONS = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; i++) {
                try {
                    std::string id = settings.id();
                    if (id.empty()) errors++;
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(errors.load(), 0);
    return true;
}

bool test_concurrent_setid() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 5;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&, t]() {
            try {
                settings.setId("thread" + std::to_string(t));
            } catch (...) {
                errors++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::string final_id = settings.id();
    ASSERT(!final_id.empty());
    ASSERT_EQ(errors.load(), 0);

    return true;
}

bool test_mixed_concurrent_operations() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 20;
    const int ITERATIONS = 50;

    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS / 2; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; i++) {
                try {
                    auto val = settings.get("key", "", SettingValue("default"));
                    std::string id = settings.id();
                    size_t s = settings.size();
                    (void)val; (void)id; (void)s;  // Suppress unused warnings
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    for (int t = 0; t < NUM_THREADS / 4; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS / 2; i++) {
                try {
                    settings.forEach([](const SettingKey&, const SettingValue&) {});
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    for (int t = 0; t < NUM_THREADS / 4; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ITERATIONS / 10; i++) {
                try {
                    settings.setId("thread" + std::to_string(t));
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(errors.load(), 0);
    return true;
}

// ============ EDGE CASES TESTS ============

bool test_edge_case_values() {
    GlobalSettings settings;

    auto val1 = settings.get("nonexistent", "", SettingValue(""));
    ASSERT_EQ(valueToStr(val1), "");

    std::string long_key(1000, 'a');
    auto val2 = settings.get(long_key, "", SettingValue("default"));
    ASSERT_EQ(valueToStr(val2), "default");

    std::string long_category(1000, 'b');
    auto val3 = settings.get("key", long_category, SettingValue("default"));
    ASSERT_EQ(valueToStr(val3), "default");

    return true;
}

bool test_special_char_values() {
    auto val1 = makeValue("test@#$%^&*");
    ASSERT_EQ(valueToStr(val1), "test@#$%^&*");

    return true;
}

// ============ MAIN TEST RUNNER ============

int main() {
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   GlobalSettings Comprehensive Test Suite   ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    TestFramework framework;

    std::cout << ">>> BASIC FUNCTIONALITY TESTS" << std::endl;
    framework.add_test("get() with default value", test_get_default);
    framework.add_test("get() with different types", test_get_different_types);
    framework.add_test("id() method", test_id_method);
    framework.add_test("setId() with valid value", test_setid_valid);
    framework.add_test("size() method", test_size_method);
    framework.add_test("forEach() method", test_foreach_method);
    framework.add_test("Multiple instances", test_multiple_instances);

    std::cout << std::endl << ">>> INPUT VALIDATION TESTS" << std::endl;
    framework.add_test("Path traversal protection", test_path_traversal_protection);
    framework.add_test("Malformed keys handled", test_malformed_keys);
    framework.add_test("Value conversions", test_value_conversions);
    framework.add_test("makeValue function", test_make_value);

    std::cout << std::endl << ">>> THREAD SAFETY TESTS" << std::endl;
    framework.add_test("Concurrent reads", test_concurrent_reads);
    framework.add_test("Concurrent id() calls", test_concurrent_id_reads);
    framework.add_test("Concurrent setId() calls", test_concurrent_setid);
    framework.add_test("Mixed concurrent operations", test_mixed_concurrent_operations);

    std::cout << std::endl << ">>> EDGE CASES TESTS" << std::endl;
    framework.add_test("Edge case values", test_edge_case_values);
    framework.add_test("Special character values", test_special_char_values);

    std::cout << std::endl;
    bool success = framework.run_all();

    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    if (success) {
        std::cout << "║           ✅ ALL TESTS PASSED! ✅            ║" << std::endl;
    } else {
        std::cout << "║          ❌ SOME TESTS FAILED! ❌            ║" << std::endl;
    }
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;

    return success ? 0 : 1;
}
