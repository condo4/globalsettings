# Contributing Guide

## Welcome

Thank you for considering contributing to GlobalSettings! This document provides guidelines for contributing to the project.

## Getting Started

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+)
- CMake 3.14+
- Git
- pybind11 (for Python bindings)
- Basic understanding of thread safety and configuration management

### Development Setup

```bash
# Clone the repository
git clone <repository-url>
cd globalsettings

# Create development build
mkdir build-dev
cd build-dev

# Build with debug symbols and warnings
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" \
         -DWITH_PYTHON=ON

make

# Run tests
./test_all
```

## Code Style

### C++ Guidelines

Follow these coding standards:

```cpp
// Use meaningful names
class ConfigurationManager {  // Good
    void loadSettings();       // Good
};

class CM {                     // Bad: Too abbreviated
    void ld();                 // Bad: Unclear
};

// Indentation: 4 spaces
if (condition) {
    doSomething();
}

// Braces on same line for functions
void myFunction() {
    // Code here
}

// Const correctness
const std::string& getValue() const {  // Good
    return value_;
}

// Member variable naming
class MyClass {
private:
    std::string value_;     // Trailing underscore for members
    static int count_;      // Also for static members
};

// Use modern C++ features
auto value = settings.get(...);              // Good: auto
for (const auto& [key, val] : map) { ... }  // Good: structured bindings

// Avoid raw pointers, prefer smart pointers
std::unique_ptr<GlobalSettings> settings_;   // Good
GlobalSettings* settings_;                   // Avoid if possible
```

### Formatting

Use consistent formatting:

```bash
# Install clang-format
sudo apt-get install clang-format

# Format code
clang-format -i src/*.cpp src/*.h
```

### Comments

```cpp
/**
 * @brief Brief description of function
 *
 * Detailed description of what this function does,
 * any important notes, edge cases, etc.
 *
 * @param key Setting key name
 * @param category Optional category
 * @return SettingValue or default
 *
 * @throws std::runtime_error If file access fails
 */
SettingValue get(const std::string& key,
                 const std::string& category = "",
                 const SettingValue& defaultValue = SettingValue()) const;

// Inline comments for complex logic
if (complex_condition) {
    // Explain why this is necessary
    handleEdgeCase();
}
```

## Making Changes

### Workflow

1. **Create a branch**
   ```bash
   git checkout -b feature/my-new-feature
   # or
   git checkout -b fix/issue-123
   ```

2. **Make changes**
   - Write code following style guidelines
   - Add tests for new features
   - Update documentation

3. **Test thoroughly**
   ```bash
   # Build and test
   cd build-dev
   make
   ./test_all

   # Run specific tests
   ./test_threads
   ./test_validation

   # Check for memory leaks
   valgrind --leak-check=full ./test_all
   ```

4. **Commit changes**
   ```bash
   git add .
   git commit -m "Add feature: description of changes"
   ```

5. **Push and create PR**
   ```bash
   git push origin feature/my-new-feature
   ```

### Commit Messages

Write clear, descriptive commit messages:

```
Good:
  - Add thread safety tests for concurrent writes
  - Fix race condition in ChangeWatcher initialization
  - Update API documentation for get() method
  - Improve error handling in file parsing

Bad:
  - Fix bug
  - Update code
  - Changes
```

Format:
```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what and why, not how.

- Bullet points are okay
- Use present tense: "Add feature" not "Added feature"

Fixes #123
```

## Adding Features

### Feature Checklist

When adding a new feature:

- [ ] Implement the feature in appropriate source files
- [ ] Add unit tests covering the feature
- [ ] Add integration tests if needed
- [ ] Update API documentation
- [ ] Add usage examples
- [ ] Update CHANGELOG
- [ ] Ensure backward compatibility (or document breaking changes)
- [ ] Test on multiple platforms if possible

### Example: Adding a New Method

```cpp
// 1. Add declaration in header (globalsettings.h)
class GlobalSettings {
public:
    /**
     * @brief Clear all settings in a category
     * @param category Category to clear
     * @return Number of settings cleared
     */
    size_t clearCategory(const std::string& category);
};

// 2. Implement in source file (globalsettings.cpp)
size_t GlobalSettings::clearCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = settings_.find(category);
    if (it == settings_.end()) {
        return 0;
    }

    size_t count = it->second.size();
    settings_.erase(it);

    return count;
}

// 3. Add test (test/test_new_feature.cpp)
bool testClearCategory() {
    GlobalSettings settings;

    settings.set("key1", SettingValue(1), "test");
    settings.set("key2", SettingValue(2), "test");

    size_t cleared = settings.clearCategory("test");
    TEST_ASSERT(cleared == 2);

    auto value = settings.get("key1", "test", SettingValue(-1));
    TEST_ASSERT(std::get<int>(value) == -1);  // Should be default

    return true;
}

// 4. Add to documentation (docs/api-reference.md)
```

## Testing

### Writing Tests

All new features must include tests:

```cpp
#include "test_framework.h"
#include "globalsettings.h"

// Test basic functionality
bool testMyFeature() {
    GlobalSettings settings;

    // Test expected behavior
    TEST_ASSERT(settings.myFeature() == expected);

    return true;
}

// Test edge cases
bool testMyFeatureEdgeCases() {
    GlobalSettings settings;

    // Empty input
    TEST_ASSERT(settings.myFeature("") == expected);

    // Null/invalid input
    // Large input
    // etc.

    return true;
}

// Test error conditions
bool testMyFeatureErrors() {
    GlobalSettings settings;

    // Should handle errors gracefully
    try {
        settings.myFeature(invalid_input);
        TEST_ASSERT(false);  // Should have thrown
    } catch (const std::exception& e) {
        // Expected
    }

    return true;
}
```

### Test Coverage

Aim for high test coverage:
- Unit tests for individual functions
- Integration tests for feature interactions
- Thread safety tests for concurrent operations
- Security tests for user input handling

## Documentation

### API Documentation

Use Doxygen-style comments:

```cpp
/**
 * @file globalsettings.h
 * @brief Thread-safe global configuration settings manager
 */

/**
 * @class GlobalSettings
 * @brief Main class for configuration management
 *
 * GlobalSettings provides a thread-safe interface for reading and
 * writing configuration settings from multiple file sources.
 */

/**
 * @brief Retrieves a setting value
 *
 * This method is thread-safe and can be called concurrently from
 * multiple threads.
 *
 * @param key Setting key name
 * @param category Optional category (default: "")
 * @param defaultValue Value to return if not found
 * @return SettingValue (variant of string, int, double, bool)
 *
 * @code
 * GlobalSettings settings;
 * auto value = settings.get("port", "network", SettingValue(8080));
 * int port = std::get<int>(value);
 * @endcode
 */
SettingValue get(const std::string& key,
                 const std::string& category = "",
                 const SettingValue& defaultValue = SettingValue()) const;
```

### User Documentation

Update relevant documentation in `docs/`:
- `api-reference.md` - For API changes
- `getting-started.md` - For user-facing changes
- `examples.md` - Add usage examples
- `troubleshooting.md` - For common issues

## Code Review

### Review Checklist

Before submitting a PR, review:

- [ ] Code follows project style guidelines
- [ ] All tests pass
- [ ] New features have tests
- [ ] Documentation is updated
- [ ] No compiler warnings
- [ ] No memory leaks (checked with valgrind)
- [ ] Thread safety considered
- [ ] Error handling is robust
- [ ] Backward compatibility maintained

### Review Process

1. Submit PR with clear description
2. Automated tests will run
3. Maintainers will review code
4. Address feedback
5. Once approved, PR will be merged

## Reporting Issues

### Bug Reports

Include:
- Clear, descriptive title
- Steps to reproduce
- Expected behavior
- Actual behavior
- Environment (OS, compiler, versions)
- Code sample if applicable
- Error messages/logs

Example:
```markdown
### Bug: Settings not persisting after restart

**Environment:**
- OS: Ubuntu 22.04
- Compiler: GCC 11.2
- GlobalSettings version: 1.1.0

**Steps to reproduce:**
1. Create settings instance: `GlobalSettings settings;`
2. Set value: `settings.set("test", SettingValue(42), "app");`
3. Restart application
4. Read value: `settings.get("test", "app", SettingValue(0));`

**Expected:** Value should be 42
**Actual:** Value is 0 (default)

**Additional info:**
- Checked /var/system.conf.d/ - file exists and contains correct value
- File permissions: 644
- Ownership declaration: @test = myapp
```

### Feature Requests

Include:
- Clear description of feature
- Use case / motivation
- Proposed API (if applicable)
- Alternatives considered

## Security

### Reporting Security Issues

**Do not open public issues for security vulnerabilities.**

Instead:
1. Email security contact (see README)
2. Include detailed description
3. Provide proof of concept if possible
4. Allow time for fix before disclosure

### Security Guidelines

When contributing:
- Validate all user input
- Avoid buffer overflows
- Prevent path traversal
- Be careful with file permissions
- Consider thread safety implications
- Review for injection vulnerabilities

## License

By contributing, you agree that your contributions will be licensed under the project's license (MIT License).

## Questions?

- Check existing documentation
- Search closed issues
- Open a discussion for questions
- Contact maintainers

## Thank You!

Your contributions make GlobalSettings better for everyone. Thank you for taking the time to contribute!
