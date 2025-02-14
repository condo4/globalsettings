#ifndef UTILSSETTINGS_H
#define UTILSSETTINGS_H

#include "globalsettings.h"

struct setting_value_functor {
    std::string operator()(const std::string &x) const { return x; }
    std::string operator()(int x) const { return std::to_string(x); }
    std::string operator()(double x) const { return std::to_string(x); }
    std::string operator()(bool x) const { return x ? "true" : "false"; }
};


static inline bool is_numeric(const std::string& str) {
    return !str.empty() &&
           str.find_first_not_of("0123456789.-") == std::string::npos &&
           (std::count(str.begin(), str.end(), '.') <= 1) &&
           (std::count(str.begin(), str.end(), '-') <= 1) &&
           (str[0] == '-' || std::isdigit(str[0]));
}

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
