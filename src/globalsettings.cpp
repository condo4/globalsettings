#include "globalsettings.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <filesystem>
#include <climits>
#include <unistd.h>
#include "changewatcher.h"
#include "utilssettings.h"

static inline void trim(std::string & str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](const unsigned char ch){return !std::isspace(ch);}));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](const unsigned char ch){return !std::isspace(ch);}).base(), str.end());
}

static inline bool list_contains(std::vector<std::string> list, const std::string & str)
{
    return (std::find(list.begin(), list.end(), str) != list.end());
}

class GlobalSettingsPrivate {
    friend class GlobalSettings;
    GlobalSettings *m_parent;
    std::map<SettingKey, SettingValue> m_data;
    ChangeWatcher m_watcher;
    std::map<SettingKey, std::string> m_owner;
    std::string m_id;
    bool m_bypass {false};
    std::vector<std::function<void(const std::string&, const std::string&, SettingValue)>> m_notifier;

public:
    GlobalSettingsPrivate(GlobalSettings *parent)
        : m_parent(parent) {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string binary = std::string(result, (count > 0) ? count : 0);
        std::filesystem::path bin = binary;
        m_id = bin.filename();

        m_watcher.setFileAdded([this](const std::string path){ refreshDirectory(path); });
        m_watcher.setFileRemoved([this](const std::string path){ refreshAll(); });
        m_watcher.setFileWrited([this](const std::string path){ refreshFile(path); });
    }

    void parseFile(const std::string &filename, std::map<SettingKey, SettingValue> &data, bool watch = false)
    {
        if(!std::filesystem::exists(filename))
        {
            return;
        }
        std::filesystem::path pfile = filename;
        if(pfile.filename().string().starts_with(".")) return;
        if(pfile.extension() != ".conf") return;

        if(watch)
        {
            m_watcher.watch(filename);
        }

        std::ifstream file(filename);
        std::string line;
        std::string category = "";

        while (std::getline(file, line)) {
            trim(line);

            if(line.empty() || line.starts_with("#") || line.ends_with(";")) {
                continue;
            }
            if(line.starts_with("[") && line.ends_with("]")) {
                category = line.substr(1, line.length() - 2);
                if(category == "General") category.clear();
                continue;
            }
            size_t pos = line.find("=");
            std::string key = line.substr(0, pos);
            trim(key);
            std::string value = line.substr(pos + 1);
            trim(value);

            if(key.starts_with("@"))
            {
                m_owner[SettingKey(category, key.substr(1))] = value;
                continue;
            }
            if(key.ends_with("/owner"))
            {
                std::cerr << "WARNING: Legacy usage for owner: " << key << " should be @" << key.substr(0, key.size() - 6) << std::endl;
                m_owner[SettingKey(category, key.substr(0, key.size() - 6))] = value;
                continue;
            }

            data[SettingKey(category, key)] = makeValue(value);
        }
    }

    void parseDir(const std::string &dconf, std::map<SettingKey, SettingValue> &data, bool watch)
    {
        if(std::filesystem::exists(dconf) && std::filesystem::is_directory(dconf))
        {
            for (const auto& entry : std::filesystem::directory_iterator(dconf))
            {
                if(entry.is_directory()) continue;
                parseFile(std::filesystem::absolute(entry.path()), data, watch);
            }
        }
    }

    void parseConf(const std::string &fconf, std::map<SettingKey, SettingValue> &data, bool watch)
    {
        if(std::filesystem::exists(fconf))
        {
            parseFile(fconf, data, watch);
        }
        std::string dconf = fconf + ".d";
        parseDir(dconf, data, watch);
        if(watch)
        {
            m_watcher.watch(dconf);
        }
    }

    void refreshData(const std::map<SettingKey, SettingValue> &cust_data)
    {
        for(const auto& [key, value] : cust_data) {
            if(!m_data.contains(key) || m_data[key] != value)
            {
                // New ITEM OR Data changed
                m_data[key] = value;
                for(auto &notifier: m_notifier)
                {
                    notifier(key.first, key.second, value);
                }
                continue;
            }
        }
    }

    void refreshFile(const std::string &file)
    {
        std::map<SettingKey, SettingValue> cust_data;
        parseFile(file, cust_data);
        refreshData(cust_data);
    }

    void refreshDirectory(const std::string &dirname)
    {
        std::map<SettingKey, SettingValue> cust_data;
        parseDir(dirname, cust_data, true);
        refreshData(cust_data);
    }

    void refreshAll()
    {
        std::map<SettingKey, SettingValue> cust_data;
        parseConf("/etc/system.conf", cust_data, false);
        parseConf("/var/system.conf", cust_data, true);
        refreshData(cust_data);
    }
};

GlobalSettings::GlobalSettings()
    : m_ptr(std::make_unique<GlobalSettingsPrivate>(this))
{
    m_ptr->parseConf("/etc/system.conf", m_ptr->m_data, false);
    m_ptr->parseConf("/var/system.conf", m_ptr->m_data, true);
}

const std::map<SettingKey, SettingValue> &GlobalSettings::data()
{
    return m_ptr->m_data;
}

SettingValue GlobalSettings::get(const std::string &key, const std::string &category, SettingValue def)
{
    std::string gkey = key;
    std::string gcategory = category;
    if(gcategory.empty() && gkey.starts_with("["))
    {
        gcategory = gkey.substr(1, gkey.find(']') - 1);
        gkey = gkey.substr(gkey.find(']') + 1);
    }

    SettingKey pkey(gcategory, gkey);

    if(m_ptr->m_data.count(pkey))
    {
        return m_ptr->m_data[pkey];
    }
    return def;
}

bool GlobalSettings::set(const std::string &key, SettingValue value, const std::string &category)
{
    // Check if variable is writable
    SettingKey pkey(category, key);
    std::string id;

    if(!m_ptr->m_owner.count(pkey))
    {
        std::cerr << "Variable " << category << ">" << key << " is not writable" << std::endl;
        return false;
    }

    if(m_ptr->m_bypass)
    {
        id = m_ptr->m_owner[pkey];
    }
    else
    {
        id = m_ptr->m_id;
        if(m_ptr->m_owner[pkey] != id)
        {
            std::cerr << "Variable " << category << ">" << key << " is not own by " << m_ptr->m_id << std::endl;
            return false;
        }
    }

    // Permission is OK, get old custom variable
    std::map<SettingKey, SettingValue> cust_data;
    m_ptr->parseFile("/var/system.conf.d/"+id+".conf", cust_data);
    cust_data[pkey] = value;
    std::string last_category = "";
    std::ofstream file("/var/system.conf.d/"+id+".conf");

    for(const auto& [key, value] : cust_data) {
        if(key.first != last_category)
        {
            last_category = key.first.empty() ? "General" : key.first;
            file << "[" << last_category << "]\n";
        }
        file << key.second << " = " << std::visit(setting_value_functor(), value) << "\n";
    }

    m_ptr->m_data[pkey] = value;
    return true;
}

std::string GlobalSettings::id()
{
    return m_ptr->m_id;
}

void GlobalSettings::setId(const std::string &newid)
{
    m_ptr->m_id = newid;
}

void GlobalSettings::bypass()
{
    if(m_ptr->m_id == "settings")
    {
        m_ptr->m_bypass = true;
    }
}

void GlobalSettings::setNotifier(const std::function<void (const std::string &, const std::string &, SettingValue)> &callback)
{
    m_ptr->m_notifier.push_back(callback);
    m_ptr->m_watcher.setEnable(true);
}

GlobalSettings::~GlobalSettings()
{
    m_ptr->m_watcher.setEnable(false);
}
