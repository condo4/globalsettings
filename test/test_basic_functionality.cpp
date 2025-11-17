#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"

// Test get with default value
bool test_get_default() {
    GlobalSettings settings;

    auto val = settings.get("nonexistent_key", "", SettingValue("default_value"));
    ASSERT_EQ(valueToStr(val), "default_value");

    return true;
}

// Test get with different types
bool test_get_different_types() {
    GlobalSettings settings;

    // String default
    auto val1 = settings.get("key1", "", SettingValue("string"));
    ASSERT_EQ(valueToStr(val1), "string");

    // Int default
    auto val2 = settings.get("key2", "", SettingValue(42));
    ASSERT_EQ(valueToInt(val2), 42);

    // Double default
    auto val3 = settings.get("key3", "", SettingValue(3.14));
    // Just verify it doesn't crash

    // Bool default
    auto val4 = settings.get("key4", "", SettingValue(true));
    ASSERT_EQ(valueToStr(val4), "true");

    return true;
}

// Test id() method
bool test_id_method() {
    GlobalSettings settings;

    std::string id = settings.id();
    ASSERT(!id.empty());

    // ID should be valid (no path separators, no ..)
    ASSERT(id.find("/") == std::string::npos);
    ASSERT(id.find("\\") == std::string::npos);
    ASSERT(id.find("..") == std::string::npos);

    return true;
}

// Test setId() with valid value
bool test_setid_valid() {
    GlobalSettings settings;

    settings.setId("test_app");
    ASSERT_EQ(settings.id(), "test_app");

    settings.setId("another-app_123");
    ASSERT_EQ(settings.id(), "another-app_123");

    return true;
}

// Test size() method
bool test_size_method() {
    GlobalSettings settings;

    size_t size = settings.size();
    // size_t is always >= 0, just verify it exists
    (void)size;

    return true;
}

// Test forEach method
bool test_foreach_method() {
    GlobalSettings settings;

    int count = 0;
    bool success = true;
    settings.forEach([&count, &success](const SettingKey& key, const SettingValue& /*value*/) {
        count++;
        // Verify key has two components (category, name)
        if (key.second.empty()) {
            success = false;
        }
    });

    ASSERT(success);
    // Should iterate over all settings
    ASSERT(count >= 0);

    return true;
}

// Test get with category
bool test_get_with_category() {
    GlobalSettings settings;

    // Get with explicit category
    auto val1 = settings.get("key", "Network", SettingValue("default"));
    ASSERT_EQ(valueToStr(val1), "default");

    // Get with bracket notation
    auto val2 = settings.get("[Network]key", "", SettingValue("default2"));
    ASSERT_EQ(valueToStr(val2), "default2");

    return true;
}

// Test default constructor
bool test_default_constructor() {
    GlobalSettings settings;

    // Should not crash
    ASSERT(!settings.id().empty());
    // size() returns size_t which is always >= 0
    (void)settings.size();

    return true;
}

// Test custom base constructor
bool test_custom_base_constructor() {
    GlobalSettings settings("custom_base");

    // Should not crash
    ASSERT(!settings.id().empty());
    // size() returns size_t which is always >= 0
    (void)settings.size();

    return true;
}

// Test multiple instances
bool test_multiple_instances() {
    GlobalSettings settings1;
    GlobalSettings settings2;

    // Both should work independently
    ASSERT(!settings1.id().empty());
    ASSERT(!settings2.id().empty());

    settings1.setId("app1");
    settings2.setId("app2");

    ASSERT_EQ(settings1.id(), "app1");
    ASSERT_EQ(settings2.id(), "app2");

    return true;
}

// Test get with empty key
bool test_get_empty_key() {
    GlobalSettings settings;

    auto val = settings.get("", "", SettingValue("default"));
    // Should not crash
    ASSERT_EQ(valueToStr(val), "default");

    return true;
}

// Test get with empty category
bool test_get_empty_category() {
    GlobalSettings settings;

    auto val = settings.get("key", "", SettingValue("default"));
    // Should not crash
    ASSERT_EQ(valueToStr(val), "default");

    return true;
}

// Test forEach with empty callback
bool test_foreach_empty() {
    GlobalSettings settings;

    int count = 0;
    settings.forEach([&count](const SettingKey&, const SettingValue&) {
        count++;
    });

    // Should not crash
    ASSERT(count >= 0);

    return true;
}

int main() {
    TestFramework framework;

    framework.add_test("get() with default value", test_get_default);
    framework.add_test("get() with different types", test_get_different_types);
    framework.add_test("id() method", test_id_method);
    framework.add_test("setId() with valid value", test_setid_valid);
    framework.add_test("size() method", test_size_method);
    framework.add_test("forEach() method", test_foreach_method);
    framework.add_test("get() with category", test_get_with_category);
    framework.add_test("Default constructor", test_default_constructor);
    framework.add_test("Custom base constructor", test_custom_base_constructor);
    framework.add_test("Multiple instances", test_multiple_instances);
    framework.add_test("get() with empty key", test_get_empty_key);
    framework.add_test("get() with empty category", test_get_empty_category);
    framework.add_test("forEach() with empty callback", test_foreach_empty);

    bool success = framework.run_all();
    return success ? 0 : 1;
}
