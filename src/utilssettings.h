/**
 * @file utilssettings.h
 * @brief Utility functions for GlobalSettings value conversion and manipulation
 * @author GlobalSettings Team
 * @version 1.1.0
 *
 * This file provides utility functions and functors for converting between
 * SettingValue variants and other types. It handles type conversions, parsing,
 * and formatting for configuration values.
 *
 * @section usage Usage Example
 * @code
 * #include "utilssettings.h"
 *
 * // Convert SettingValue to string
 * SettingValue val = 42;
 * std::string str = valueToStr(val);  // "42"
 *
 * // Convert SettingValue to int
 * SettingValue val2 = "123";
 * int num = valueToInt(val2);  // 123
 *
 * // Parse string to SettingValue
 * SettingValue val3 = makeValue("true");  // bool(true)
 * SettingValue val4 = makeValue("3.14");  // double(3.14)
 * @endcode
 *
 * @note All functions are inline for performance
 * @note Header-only library
 */

#ifndef UTILSSETTINGS_H
#define UTILSSETTINGS_H

#include "globalsettings.h"
#include <iostream>
#include <algorithm>

/**
 * @brief Functor to convert SettingValue variants to string
 *
 * Provides string conversion for all supported setting value types using
 * std::visit pattern. Used internally by valueToStr().
 *
 * Conversion rules:
 * - string -> string (unchanged)
 * - int -> decimal string
 * - double -> decimal string with fractional part
 * - bool -> "true" or "false"
 */
struct settingValueFunctor {
    std::string operator()(const std::string &x) const { return x; }  ///< String passthrough
    std::string operator()(int x) const { return std::to_string(x); }  ///< Int to string
    std::string operator()(double x) const { return std::to_string(x); }  ///< Double to string
    std::string operator()(bool x) const { return x ? "true" : "false"; }  ///< Bool to "true"/"false"
};

/**
 * @brief Convert a SettingValue into a string for display
 *
 * Converts any SettingValue type to its string representation. This is
 * useful for logging, debugging, or displaying values to users.
 *
 * @param value Input SettingValue to convert
 * @return String representation of the value
 *
 * @note Always succeeds (never throws)
 * @note Safe for all SettingValue types
 *
 * Example:
 * @code
 * SettingValue val1 = 42;
 * SettingValue val2 = 3.14;
 * SettingValue val3 = true;
 * std::cout << valueToStr(val1) << std::endl;  // "42"
 * std::cout << valueToStr(val2) << std::endl;  // "3.140000"
 * std::cout << valueToStr(val3) << std::endl;  // "true"
 * @endcode
 */
static inline std::string valueToStr(const SettingValue &value)
{
    return std::visit(settingValueFunctor(), value);
}

/**
 * @brief Functor to convert SettingValue variants to int
 *
 * Provides int conversion for all supported setting value types using
 * std::visit pattern. Used internally by valueToInt().
 *
 * Conversion rules:
 * - string -> parsed as integer (0 if invalid)
 * - int -> int (unchanged)
 * - double -> truncated to int
 * - bool -> 1 (true) or 0 (false)
 *
 * @note Invalid string conversions print warning and return 0
 */
struct settingValueFunctorInt {
    /**
     * @brief Convert string to int
     * @param x String value
     * @return Parsed integer or 0 if parsing fails
     */
    int operator()(const std::string &x) const {
        try {
            return std::stoi(x);
        } catch (const std::exception& e) {
            std::cerr << "WARNING: Cannot convert to int: " << x << std::endl;
            return 0;
        }
    }
    int operator()(int x) const { return x; }  ///< Int passthrough
    int operator()(double x) const { return static_cast<int>(x); }  ///< Double truncation
    int operator()(bool x) const { return x ? 1 : 0; }  ///< Bool to 0/1
};

/**
 * @brief Convert a SettingValue into an integer
 *
 * Converts any SettingValue type to an integer. For strings, attempts
 * to parse as a number. For doubles, truncates to int.
 *
 * @param value Input SettingValue to convert
 * @return Integer representation of the value
 *
 * @note Returns 0 for invalid string conversions (with warning)
 * @note Truncates doubles without rounding
 * @note Safe for all SettingValue types
 *
 * Example:
 * @code
 * SettingValue val1 = "123";
 * SettingValue val2 = 45.67;
 * SettingValue val3 = true;
 * int n1 = valueToInt(val1);  // 123
 * int n2 = valueToInt(val2);  // 45
 * int n3 = valueToInt(val3);  // 1
 * @endcode
 */
static inline int valueToInt(const SettingValue &value)
{
    return std::visit(settingValueFunctorInt(), value);
}
/**
 * @brief Check if a string represents a valid numeric value
 *
 * Validates that a string contains only numeric characters and can be
 * parsed as a number (integer or floating-point).
 *
 * @param str String to check
 * @return true if string is numeric, false otherwise
 *
 * @note Accepts integers and floating-point numbers
 * @note Allows negative numbers (leading '-')
 * @note Allows at most one decimal point
 * @note Empty strings are not numeric
 *
 * Examples:
 * @code
 * is_numeric("123")      // true
 * is_numeric("-45.67")   // true
 * is_numeric("3.14")     // true
 * is_numeric("abc")      // false
 * is_numeric("")         // false
 * is_numeric("12.34.56") // false (multiple dots)
 * @endcode
 */
static inline bool is_numeric(const std::string& str) {
    return !str.empty() &&
           str.find_first_not_of("0123456789.-") == std::string::npos &&
           (std::count(str.begin(), str.end(), '.') <= 1) &&
           (std::count(str.begin(), str.end(), '-') <= 1) &&
           (str[0] == '-' || std::isdigit(str[0]));
}

/**
 * @brief Convert a string to appropriate SettingValue type
 *
 * Intelligently parses a string value and converts it to the most appropriate
 * SettingValue type (string, int, double, or bool). This is the primary function
 * for parsing configuration file values.
 *
 * @param value Input string to convert
 * @return SettingValue containing the appropriately-typed value
 *
 * @note Case-insensitive for boolean values
 * @note Prints warnings for malformed quoted strings
 * @note Safe to call with any input (never throws)
 *
 * Conversion rules (in order of precedence):
 * 1. Quoted strings ("..." or '...') -> string without quotes
 * 2. Numeric values -> int (no decimal) or double (with decimal)
 * 3. "true", "on" (case-insensitive) -> boolean true
 * 4. "false", "off" (case-insensitive) -> boolean false
 * 5. Everything else -> string (unquoted)
 *
 * Examples:
 * @code
 * makeValue("\"hello\"")  // string("hello")
 * makeValue("123")        // int(123)
 * makeValue("3.14")       // double(3.14)
 * makeValue("true")       // bool(true)
 * makeValue("TRUE")       // bool(true)
 * makeValue("on")         // bool(true)
 * makeValue("false")      // bool(false)
 * makeValue("hello")      // string("hello")
 * @endcode
 */
static inline SettingValue makeValue(const std::string &value)
{
    if(value.starts_with('"') || value.starts_with('\''))
    {
        if (value.length() < 2) {
            std::cerr << "WARNING: Invalid quoted value: " << value << std::endl;
            return SettingValue(std::string(""));
        }
        char quote = value[0];
        if (value.back() != quote) {
            std::cerr << "WARNING: Mismatched quotes in value: " << value << std::endl;
            return SettingValue(std::string(""));
        }
        return value.substr(1, value.length() - 2);
    }

    if(is_numeric(value))
    {
        try {
            // double
            if(value.find('.') != value.npos)
            {
                return std::stod(value);
            }
            else
            {
                return std::stoi(value);
            }
        } catch (const std::exception& e) {
            std::cerr << "WARNING: Numeric parsing failed for '" << value << "', treating as string" << std::endl;
            return SettingValue(value);
        }
    }

    // To Lower case
    std::string lvalue = value;
    std::transform(lvalue.begin(), lvalue.end(), lvalue.begin(),
                   [](unsigned char c){ return std::tolower(c); });


    // bool true
    if(lvalue == "true" || lvalue == "on")
    {
        return true;
    }

    // bool false
    if(lvalue == "false" || lvalue == "off")
    {
        return false;
    }

    // else, string without quote
    return value;
}

#endif // UTILSSETTINGS_H
