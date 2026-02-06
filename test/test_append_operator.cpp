#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"
#include <fstream>
#include <filesystem>

// Test string concatenation with no spaces
bool test_string_append_no_spaces() {
    GlobalSettings settings;

    auto result = settings.get("test_a", "", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(result), "foo bar");

    return true;
}

// Test string concatenation with leading space
bool test_string_append_leading_space() {
    GlobalSettings settings;

    auto result = settings.get("test_b", "", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(result), "foo bar");

    return true;
}

// Test string concatenation with trailing space
bool test_string_append_trailing_space() {
    GlobalSettings settings;

    auto result = settings.get("test_c", "", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(result), "foo bar");

    return true;
}

// Test integer addition
bool test_integer_append() {
    GlobalSettings settings;

    auto result = settings.get("test_d", "", SettingValue(0));
    ASSERT_EQ(std::get<int>(result), 9);

    return true;
}

// Test multiple string appends
bool test_multiple_string_appends() {
    GlobalSettings settings;

    auto result = settings.get("test_multi", "", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(result), "one two three");

    return true;
}

// Test multiple integer appends
bool test_multiple_integer_appends() {
    GlobalSettings settings;

    auto result = settings.get("test_sum", "", SettingValue(0));
    ASSERT_EQ(std::get<int>(result), 15);

    return true;
}

// Test double addition
bool test_double_append() {
    GlobalSettings settings;

    auto result = settings.get("test_double", "", SettingValue(0.0));
    ASSERT_EQ(std::get<double>(result), 8.5);

    return true;
}

// Test mixed int and double
bool test_mixed_numeric_append() {
    GlobalSettings settings;

    auto result = settings.get("test_mixed", "", SettingValue(0.0));
    // Should be double: 5 + 2.5 = 7.5
    ASSERT(std::holds_alternative<double>(result));
    ASSERT_EQ(std::get<double>(result), 7.5);

    return true;
}

// Test append in different category
bool test_append_with_category() {
    GlobalSettings settings;

    auto result = settings.get("category_test", "TestCategory", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(result), "hello world");

    return true;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        GlobalSettings += Operator Tests                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    TestFramework framework;

    std::cout << ">>> STRING APPEND TESTS" << std::endl;
    framework.add_test("String append (no spaces)", test_string_append_no_spaces);
    framework.add_test("String append (leading space)", test_string_append_leading_space);
    framework.add_test("String append (trailing space)", test_string_append_trailing_space);
    framework.add_test("Multiple string appends", test_multiple_string_appends);
    framework.add_test("Append with category", test_append_with_category);

    std::cout << std::endl << ">>> NUMERIC APPEND TESTS" << std::endl;
    framework.add_test("Integer addition", test_integer_append);
    framework.add_test("Multiple integer additions", test_multiple_integer_appends);
    framework.add_test("Double addition", test_double_append);
    framework.add_test("Mixed int/double addition", test_mixed_numeric_append);

    std::cout << std::endl;
    bool success = framework.run_all();

    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    if (success) {
        std::cout << "║          ✅ ALL APPEND TESTS PASSED! ✅                 ║" << std::endl;
    } else {
        std::cout << "║          ❌ SOME APPEND TESTS FAILED! ❌                ║" << std::endl;
    }
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;

    return success ? 0 : 1;
}
