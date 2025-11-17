#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"

// Test path traversal protection in setId()
bool test_path_traversal_protection() {
    GlobalSettings settings;
    std::string original_id = settings.id();

    // Test 1: Parent directory traversal
    settings.setId("../../etc/passwd");
    ASSERT_EQ(settings.id(), original_id);

    // Test 2: Absolute path
    settings.setId("/etc/malicious");
    ASSERT_EQ(settings.id(), original_id);

    // Test 3: Windows-style path
    settings.setId("..\\..\\windows\\system32");
    ASSERT_EQ(settings.id(), original_id);

    // Test 4: Command injection attempt
    settings.setId("malicious;rm -rf /");
    ASSERT_EQ(settings.id(), original_id);

    // Test 5: Special characters
    settings.setId("test@#$%");
    ASSERT_EQ(settings.id(), original_id);

    // Test 6: Empty string
    settings.setId("");
    ASSERT_EQ(settings.id(), original_id);

    // Test 7: Hidden traversal
    settings.setId("foo/../bar");
    ASSERT_EQ(settings.id(), original_id);

    return true;
}

// Test valid IDs are accepted
bool test_valid_ids() {
    GlobalSettings settings;

    // Test 1: Alphanumeric
    settings.setId("valid_app_123");
    ASSERT_EQ(settings.id(), "valid_app_123");

    // Test 2: With dashes
    settings.setId("my-app-v2");
    ASSERT_EQ(settings.id(), "my-app-v2");

    // Test 3: Alphanumeric only
    settings.setId("MyApp");
    ASSERT_EQ(settings.id(), "MyApp");

    // Test 4: Numbers and underscore
    settings.setId("app_123");
    ASSERT_EQ(settings.id(), "app_123");

    return true;
}

// Test malformed keys in get()
bool test_malformed_keys() {
    GlobalSettings settings;

    // Missing closing bracket
    auto val1 = settings.get("[category", "", SettingValue("default"));
    // Should not crash, returns default
    ASSERT_EQ(valueToStr(val1), "default");

    // Valid key with category
    auto val2 = settings.get("[category]key", "", SettingValue("test"));
    // Should not crash
    ASSERT_EQ(valueToStr(val2), "test");

    // Empty key
    auto val3 = settings.get("", "", SettingValue("empty"));
    ASSERT_EQ(valueToStr(val3), "empty");

    return true;
}

// Test value conversions
bool test_value_conversions() {
    // Test string to int
    SettingValue str_val("123");
    ASSERT_EQ(valueToInt(str_val), 123);

    // Test invalid string to int (should return 0 with warning)
    SettingValue invalid_str("not_a_number");
    ASSERT_EQ(valueToInt(invalid_str), 0);

    // Test int to string
    SettingValue int_val(42);
    ASSERT_EQ(valueToStr(int_val), "42");

    // Test double to string
    SettingValue double_val(3.14);
    std::string double_str = valueToStr(double_val);
    ASSERT(double_str.find("3.14") != std::string::npos);

    // Test bool to string
    SettingValue bool_val(true);
    ASSERT_EQ(valueToStr(bool_val), "true");

    return true;
}

// Test makeValue with various inputs
bool test_make_value() {
    // Test quoted string
    auto val1 = makeValue("\"hello world\"");
    ASSERT_EQ(valueToStr(val1), "hello world");

    // Test single quoted
    auto val2 = makeValue("'test'");
    ASSERT_EQ(valueToStr(val2), "test");

    // Test integer
    auto val3 = makeValue("42");
    ASSERT_EQ(valueToInt(val3), 42);

    // Test double
    auto val4 = makeValue("3.14");
    std::string val4_str = valueToStr(val4);
    ASSERT(val4_str.find("3.14") != std::string::npos);

    // Test boolean true
    auto val5 = makeValue("true");
    ASSERT_EQ(valueToStr(val5), "true");

    // Test boolean false
    auto val6 = makeValue("false");
    ASSERT_EQ(valueToStr(val6), "false");

    // Test "on"
    auto val7 = makeValue("on");
    ASSERT_EQ(valueToStr(val7), "true");

    // Test "off"
    auto val8 = makeValue("off");
    ASSERT_EQ(valueToStr(val8), "false");

    // Test mismatched quotes (should handle gracefully)
    auto val9 = makeValue("\"unclosed");
    // Should not crash

    // Test invalid quoted value
    auto val10 = makeValue("\"");
    // Should not crash

    return true;
}

// Test empty and edge case values
bool test_edge_case_values() {
    GlobalSettings settings;

    // Empty string value
    auto val1 = settings.get("nonexistent", "", SettingValue(""));
    ASSERT_EQ(valueToStr(val1), "");

    // Very long key
    std::string long_key(1000, 'a');
    auto val2 = settings.get(long_key, "", SettingValue("default"));
    ASSERT_EQ(valueToStr(val2), "default");

    // Very long category
    std::string long_category(1000, 'b');
    auto val3 = settings.get("key", long_category, SettingValue("default"));
    ASSERT_EQ(valueToStr(val3), "default");

    return true;
}

// Test special characters in values
bool test_special_char_values() {
    // Test value with special chars
    auto val1 = makeValue("test@#$%^&*");
    ASSERT_EQ(valueToStr(val1), "test@#$%^&*");

    // Test value with quotes inside
    auto val2 = makeValue("test with \" quote");
    // Should handle gracefully

    // Test value with newlines
    auto val3 = makeValue("line1\nline2");
    // Should handle gracefully

    // Test value with tabs
    auto val4 = makeValue("tab\there");
    // Should handle gracefully

    return true;
}

int main() {
    TestFramework framework;

    framework.add_test("Path traversal protection", test_path_traversal_protection);
    framework.add_test("Valid IDs accepted", test_valid_ids);
    framework.add_test("Malformed keys handled", test_malformed_keys);
    framework.add_test("Value conversions", test_value_conversions);
    framework.add_test("makeValue function", test_make_value);
    framework.add_test("Edge case values", test_edge_case_values);
    framework.add_test("Special character values", test_special_char_values);

    bool success = framework.run_all();
    return success ? 0 : 1;
}
