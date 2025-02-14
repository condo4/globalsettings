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

class GlobalSettingsPrivate;

class GlobalSettings
{
public:
    explicit GlobalSettings();
    virtual ~GlobalSettings();

    const std::map<SettingKey, SettingValue>& data();
    SettingValue get(const std::string &key, const std::string &category = "", SettingValue def = SettingValue());
    bool set(const std::string &key, SettingValue value, const std::string &category = "");

    std::string id();
    void setId(const std::string &newid);

    void bypass(); // Internal usage; except settings command line tool, you shouldn't use it
    void setNotifier(const std::function<void(const std::string&, const std::string&, SettingValue)>& callback);

private:
    std::unique_ptr<GlobalSettingsPrivate> m_ptr;
};

#endif // GLOBALSETTINGS_H
