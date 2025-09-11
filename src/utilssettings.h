#ifndef UTILSSETTINGS_H
#define UTILSSETTINGS_H

#include "globalsettings.h"

/**
 * @brief Functor to convert SettingValue variants to string
 *
 * Provides string conversion for all supported setting value types
 */
struct settingValueFunctor {
    std::string operator()(const std::string &x) const { return x; }
    std::string operator()(int x) const { return std::to_string(x); }
    std::string operator()(double x) const { return std::to_string(x); }
    std::string operator()(bool x) const { return x ? "true" : "false"; }
};

/**
 * @brief Convert a SettingValue into a string for display
 * @param value Input SettingValue to convert
 * @return std::string containing the converted value
 */
static inline std::string valueToStr(const SettingValue &value)
{
    return std::visit(settingValueFunctor(), value);
}

/**
 * @brief Functor to convert SettingValue variants to string
 *
 * Provides string conversion for all supported setting value types
 */
struct settingValueFunctorInt {
    int operator()(const std::string &x) const { return std::stoi(x); }
    int operator()(int x) const { return x; }
    int operator()(double x) const { return x; }
    int operator()(bool x) const { return x ? 1 : 0; }
};

/**
 * @brief Convert a SettingValue into a string for display
 * @param value Input SettingValue to convert
 * @return std::string containing the converted value
 */
static inline int valueToInt(const SettingValue &value)
{
    return std::visit(settingValueFunctorInt(), value);
}
/**
 * @brief Check if a string represents a valid numeric value
 * @param str String to check
 * @return true if string is numeric, false otherwise
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
 * @param value Input string to convert
 * @return SettingValue containing the converted value
 *
 * Conversion rules:
 * - Quoted strings -> string without quotes
 * - Numeric values -> int or double
 * - "true"/"on" -> boolean true
 * - "false"/"off" -> boolean false
 * - Other -> string
 */
static inline SettingValue makeValue(const std::string &value)
{
    if(value.starts_with('"') || value.starts_with('\''))
    {
        return value.substr(1, value.length() - 2);
    }

    if(is_numeric(value))
    {
        // double
        if(value.find('.') != value.npos)
        {
            return std::stod(value);
        }
        else
        {
            return std::stoi(value);
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
