#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <functional>
#include <memory>
#include <variant>
#include <string>
#include <map>
#include <algorithm>

using SettingKey = std::pair<std::string, std::string>;
using SettingValue = std::variant<std::string, int, double, bool>;

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

SettingValue makeValue(const std::string &value)
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

class GlobalSettingsPrivate;

class GlobalSettings
{
public:
    explicit GlobalSettings();
    virtual ~GlobalSettings();

    const std::map<SettingKey, SettingValue>& data();
    SettingValue get(const std::string &key, const std::string &category = "", SettingValue def = SettingValue());
    bool set(const std::string &key, SettingValue value, const std::string &category = "");

    void bypass(); // Internal usage; except settings command line tool, you shouldn't use it
    void setId(const std::string &newid);

    void setNotifier(const std::function<void(const std::string&, const std::string&, SettingValue)>& callback);

private:
    std::unique_ptr<GlobalSettingsPrivate> m_ptr;
};

#endif // GLOBALSETTINGS_H
