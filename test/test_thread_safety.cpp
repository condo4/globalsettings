#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

// Test concurrent reads
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
                    // Should not crash
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

// Test concurrent id() calls
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
                    ASSERT(!id.empty());
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

// Test concurrent setId() calls
bool test_concurrent_setid() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 5;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&, t]() {
            try {
                settings.setId("thread" + std::to_string(t));
                // Should not crash
            } catch (...) {
                errors++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify final ID is valid
    std::string final_id = settings.id();
    ASSERT(!final_id.empty());
    ASSERT_EQ(errors.load(), 0);

    return true;
}

// Test concurrent forEach
bool test_concurrent_foreach() {
    GlobalSettings settings;
    std::atomic<int> errors{0};
    std::atomic<int> iterations{0};

    const int NUM_THREADS = 10;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&]() {
            try {
                settings.forEach([&](const SettingKey& key, const SettingValue& value) {
                    iterations++;
                });
            } catch (...) {
                errors++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(errors.load(), 0);
    return true;
}

// Test size() under concurrent access
bool test_concurrent_size() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 10;
    const int ITERATIONS = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; i++) {
                try {
                    size_t s = settings.size();
                    // Should not crash
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

// Test mixed concurrent operations
bool test_mixed_concurrent_operations() {
    GlobalSettings settings;
    std::atomic<int> errors{0};

    const int NUM_THREADS = 20;
    const int ITERATIONS = 50;

    std::vector<std::thread> threads;

    // Reader threads
    for (int t = 0; t < NUM_THREADS / 2; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; i++) {
                try {
                    auto val = settings.get("key", "", SettingValue("default"));
                    std::string id = settings.id();
                    size_t s = settings.size();
                } catch (...) {
                    errors++;
                }
            }
        });
    }

    // forEach threads
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

    // setId threads
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

// Run all thread safety tests
int main() {
    TestFramework framework;

    framework.add_test("Concurrent reads", test_concurrent_reads);
    framework.add_test("Concurrent id() calls", test_concurrent_id_reads);
    framework.add_test("Concurrent setId() calls", test_concurrent_setid);
    framework.add_test("Concurrent forEach", test_concurrent_foreach);
    framework.add_test("Concurrent size() calls", test_concurrent_size);
    framework.add_test("Mixed concurrent operations", test_mixed_concurrent_operations);

    bool success = framework.run_all();
    return success ? 0 : 1;
}
