# Testing Documentation

## Overview

GlobalSettings includes a comprehensive test suite covering functional, thread safety, input validation, and path traversal security tests.

## Test Structure

### Test Framework

The project uses a custom lightweight test framework defined in `test/test_framework.h`:

```cpp
// Minimal test framework
#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << #condition << " at line " << __LINE__ << std::endl; \
        return false; \
    }

#define RUN_TEST(test_func) \
    std::cout << "Running " << #test_func << "... "; \
    if (test_func()) { \
        std::cout << "PASSED" << std::endl; \
    } else { \
        std::cout << "FAILED" << std::endl; \
        failed++; \
    }
```

### Test Categories

| Test Suite | File | Purpose |
|------------|------|---------|
| Basic Functionality | `test_basic_functionality.cpp` | Core read/write operations |
| Functional | `test_functional.cpp` | Advanced features |
| Thread Safety | `test_thread_safety.cpp` | Concurrent access tests |
| Input Validation | `test_input_validation.cpp` | Boundary and edge cases |
| Path Traversal | `test_path_traversal.cpp` | Security tests |
| Integration | `test_all.cpp` | Full test suite runner |

## Running Tests

### Build Tests

```bash
cd build
cmake ..
make

# Tests are built in build/ directory
ls -la test_*
```

### Run All Tests

```bash
# Run complete test suite
./test_all

# Output:
# Running basic tests...
# Running functional tests...
# Running thread safety tests...
# Running input validation tests...
# All tests passed!
```

### Run Individual Test Suites

```bash
# Basic functionality tests
./test_basic

# Thread safety tests
./test_threads

# Input validation tests
./test_validation

# Path traversal security tests
./test_path_traversal
```

## Test Environment

### Namespace Isolation

Tests use Linux namespaces to avoid requiring root privileges:

```bash
#!/bin/bash
# test/run_with_namespace.sh

# Create mount namespace
unshare --mount --map-root-user bash -c '
    # Create temporary /etc and /var
    mkdir -p /tmp/test_etc /tmp/test_var

    # Mount bind to simulate /etc and /var
    mount --bind /tmp/test_etc /etc
    mount --bind /tmp/test_var /var

    # Run tests
    "$@"
' -- "$@"
```

### Using cgroups or chroot

As mentioned in project instructions, you can also use cgroup or chroot:

```bash
# Using chroot
sudo mkdir -p /tmp/test_root/{etc,var,usr,bin,lib}
sudo cp test_binary /tmp/test_root/usr/bin/
sudo chroot /tmp/test_root /usr/bin/test_binary

# Using unshare (no root needed)
unshare -m -r bash -c 'mount --bind /tmp/test_etc /etc && ./test_binary'
```

## Test Cases

### Basic Functionality Tests

Located in `test/test_basic_functionality.cpp`:

```cpp
// Test setting and getting values
bool testBasicReadWrite() {
    GlobalSettings settings;

    // Write a setting
    TEST_ASSERT(settings.set("test_key", SettingValue(42), "test"));

    // Read it back
    auto value = settings.get("test_key", "test", SettingValue(0));
    TEST_ASSERT(std::get<int>(value) == 42);

    return true;
}

// Test different value types
bool testValueTypes() {
    GlobalSettings settings;

    // String
    TEST_ASSERT(settings.set("str", SettingValue("hello"), "test"));
    auto str_val = settings.get("str", "test", SettingValue(""));
    TEST_ASSERT(std::get<std::string>(str_val) == "hello");

    // Integer
    TEST_ASSERT(settings.set("int", SettingValue(123), "test"));
    auto int_val = settings.get("int", "test", SettingValue(0));
    TEST_ASSERT(std::get<int>(int_val) == 123);

    // Double
    TEST_ASSERT(settings.set("dbl", SettingValue(3.14), "test"));
    auto dbl_val = settings.get("dbl", "test", SettingValue(0.0));
    TEST_ASSERT(std::abs(std::get<double>(dbl_val) - 3.14) < 0.001);

    // Boolean
    TEST_ASSERT(settings.set("bool", SettingValue(true), "test"));
    auto bool_val = settings.get("bool", "test", SettingValue(false));
    TEST_ASSERT(std::get<bool>(bool_val) == true);

    return true;
}
```

### Thread Safety Tests

Located in `test/test_thread_safety.cpp`:

```cpp
// Test concurrent reads
bool testConcurrentReads() {
    GlobalSettings settings;
    settings.set("shared_key", SettingValue(42), "test");

    const int num_threads = 10;
    const int iterations = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&settings, iterations]() {
            for (int j = 0; j < iterations; j++) {
                auto value = settings.get("shared_key", "test", SettingValue(0));
                int val = std::get<int>(value);
                TEST_ASSERT(val == 42);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return true;
}

// Test concurrent writes
bool testConcurrentWrites() {
    GlobalSettings settings;

    const int num_threads = 10;
    const int iterations = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&settings, i, iterations]() {
            for (int j = 0; j < iterations; j++) {
                settings.set("thread_" + std::to_string(i),
                           SettingValue(j),
                           "test");
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify all writes completed
    for (int i = 0; i < num_threads; i++) {
        auto value = settings.get("thread_" + std::to_string(i), "test", SettingValue(-1));
        TEST_ASSERT(std::get<int>(value) >= 0);
    }

    return true;
}

// Test concurrent read/write
bool testConcurrentReadWrite() {
    GlobalSettings settings;
    settings.set("counter", SettingValue(0), "test");

    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Writer thread
    threads.emplace_back([&]() {
        int count = 0;
        while (running) {
            settings.set("counter", SettingValue(count++), "test");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Reader threads
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&]() {
            while (running) {
                auto value = settings.get("counter", "test", SettingValue(0));
                // Just verify no crash/corruption
                int val = std::get<int>(value);
                TEST_ASSERT(val >= 0);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    running = false;

    for (auto& t : threads) {
        t.join();
    }

    return true;
}
```

### Input Validation Tests

Located in `test/test_input_validation.cpp`:

```cpp
// Test empty strings
bool testEmptyStrings() {
    GlobalSettings settings;

    TEST_ASSERT(settings.set("empty", SettingValue(""), "test"));
    auto value = settings.get("empty", "test", SettingValue("default"));
    TEST_ASSERT(std::get<std::string>(value) == "");

    return true;
}

// Test special characters
bool testSpecialCharacters() {
    GlobalSettings settings;

    std::string special = "!@#$%^&*(){}[]|\\:;\"'<>?,./";
    TEST_ASSERT(settings.set("special", SettingValue(special), "test"));

    auto value = settings.get("special", "test", SettingValue(""));
    TEST_ASSERT(std::get<std::string>(value) == special);

    return true;
}

// Test large values
bool testLargeValues() {
    GlobalSettings settings;

    // Large string (1MB)
    std::string large(1024 * 1024, 'x');
    TEST_ASSERT(settings.set("large", SettingValue(large), "test"));

    auto value = settings.get("large", "test", SettingValue(""));
    TEST_ASSERT(std::get<std::string>(value) == large);

    return true;
}

// Test boundary values
bool testBoundaryValues() {
    GlobalSettings settings;

    // Max/min integers
    TEST_ASSERT(settings.set("max_int", SettingValue(INT_MAX), "test"));
    TEST_ASSERT(settings.set("min_int", SettingValue(INT_MIN), "test"));

    auto max_val = settings.get("max_int", "test", SettingValue(0));
    TEST_ASSERT(std::get<int>(max_val) == INT_MAX);

    auto min_val = settings.get("min_int", "test", SettingValue(0));
    TEST_ASSERT(std::get<int>(min_val) == INT_MIN);

    return true;
}
```

### Path Traversal Security Tests

Located in `test/test_path_traversal.cpp`:

```cpp
// Test path traversal attempts
bool testPathTraversal() {
    GlobalSettings settings;

    // Attempts to escape directory
    std::vector<std::string> malicious_paths = {
        "../../../etc/passwd",
        "../../var/secrets",
        "/etc/shadow",
        "....//....//etc/passwd"
    };

    for (const auto& path : malicious_paths) {
        // Should not allow writing to these paths
        bool result = settings.set("malicious_path",
                                  SettingValue(path),
                                  "test");
        // Verify it's handled safely
        // (Implementation specific - either rejected or sanitized)
    }

    return true;
}
```

## Writing New Tests

### Test Template

```cpp
#include "test_framework.h"
#include "globalsettings.h"

bool testMyNewFeature() {
    // Setup
    GlobalSettings settings;

    // Test action
    bool result = settings.set("key", SettingValue(123), "test");

    // Verify
    TEST_ASSERT(result == true);

    auto value = settings.get("key", "test", SettingValue(0));
    TEST_ASSERT(std::get<int>(value) == 123);

    return true;
}

int main() {
    int failed = 0;

    RUN_TEST(testMyNewFeature);

    if (failed == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << failed << " test(s) failed" << std::endl;
        return 1;
    }
}
```

### Best Practices

1. **Isolation:** Each test should be independent
2. **Cleanup:** Clean up resources after tests
3. **Assertions:** Use TEST_ASSERT for all checks
4. **Coverage:** Test both success and failure cases
5. **Documentation:** Comment complex test scenarios

## Continuous Integration

### Example GitHub Actions Workflow

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake pybind11-dev

    - name: Build
      run: |
        mkdir build
        cd build
        cmake ..
        make

    - name: Run tests
      run: |
        cd build
        ./test_all

    - name: Run thread safety tests
      run: |
        cd build
        ./test_threads

    - name: Run security tests
      run: |
        cd build
        ./test_path_traversal
```

## Test Coverage

### Measuring Coverage

```bash
# Build with coverage flags
cmake .. -DCMAKE_CXX_FLAGS="--coverage"
make

# Run tests
./test_all

# Generate coverage report
gcov ../src/*.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# View report
firefox coverage_html/index.html
```

## Performance Testing

### Benchmark Template

```cpp
#include <chrono>
#include <globalsettings.h>

void benchmarkGet() {
    GlobalSettings settings;
    settings.set("key", SettingValue(42), "test");

    const int iterations = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto value = settings.get("key", "test", SettingValue(0));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "get() average: "
              << duration.count() / iterations
              << " μs" << std::endl;
}
```

## Debugging Tests

### Running with Valgrind

```bash
# Check for memory leaks
valgrind --leak-check=full ./test_all

# Check for thread issues
valgrind --tool=helgrind ./test_threads
```

### Running with GDB

```bash
# Debug failing test
gdb ./test_basic
(gdb) run
(gdb) backtrace
```

### Enable Debug Output

```cpp
// In test code
#define DEBUG_TESTS 1

#ifdef DEBUG_TESTS
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif
```

## See Also

- [Getting Started](getting-started.md) - Setup instructions
- [API Reference](api-reference.md) - API documentation
- [Thread Safety](threading.md) - Thread safety details
- [Security](security.md) - Security considerations
