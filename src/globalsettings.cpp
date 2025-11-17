#include "globalsettings.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <filesystem>
#include <climits>
#include <mutex>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include "changewatcher.h"
#include "utilssettings.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static inline void trim(std::string & str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](const unsigned char ch){return !std::isspace(ch);}));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](const unsigned char ch){return !std::isspace(ch);}).base(), str.end());
}

static inline bool list_contains(std::vector<std::string> list, const std::string & str)
{
    return (std::find(list.begin(), list.end(), str) != list.end());
}

/**
 * @brief Validates that an ID is safe for use in file paths
 * @param id The ID string to validate
 * @return true if ID is safe (alphanumeric, dash, underscore only), false otherwise
 *
 * Prevents path traversal attacks by rejecting IDs containing:
 * - ".." (parent directory)
 * - "/" or "\" (path separators)
 * - Any non-alphanumeric characters except dash and underscore
 */
static inline bool isValidId(const std::string& id)
{
    if (id.empty()) {
        return false;
    }

    // Reject path traversal attempts
    if (id.find("..") != std::string::npos) {
        return false;
    }

    // Reject path separators
    if (id.find('/') != std::string::npos || id.find('\\') != std::string::npos) {
        return false;
    }

    // Only allow alphanumeric, dash, and underscore
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

class GlobalSettingsPrivate {
    friend class GlobalSettings;
    GlobalSettings *m_parent;
    std::map<SettingKey, SettingValue> m_data;
    ChangeWatcher m_watcher;
    std::map<SettingKey, std::string> m_owner;
    std::string m_id;
    std::string m_base;
    std::mutex m_mutex_data;
    std::mutex m_mutex_notifier;
    bool m_bypass {false};
    std::vector<std::function<void(const std::string&, const std::string&, SettingValue)>> m_notifier;

public:
    GlobalSettingsPrivate(GlobalSettings *parent, const std::string &base)
        : m_parent(parent), m_base(base) {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count <= 0 || count >= PATH_MAX) {
            std::cerr << "WARNING: Cannot determine application path, using 'unknown' as ID" << std::endl;
            m_id = "unknown";
        } else {
            std::string binary = std::string(result, count);
            std::filesystem::path bin = binary;
            m_id = bin.filename();

            // Validate ID to prevent path traversal (defense in depth)
            if (!isValidId(m_id)) {
                std::cerr << "WARNING: Binary name contains unsafe characters: \"" << m_id << "\"" << std::endl;
                std::cerr << "         Using 'unknown' as ID instead" << std::endl;
                m_id = "unknown";
            }

            if(m_id == "settings")
            {
                // Only "settings cli tools can bypass the owner protection"
                m_bypass = true;
            }
        }

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
            if (pos == std::string::npos) {
                std::cerr << "WARNING: Malformed config line (missing '='): " << line << std::endl;
                continue;
            }
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
        std::lock_guard<std::mutex> lock(m_mutex_data);
        std::lock_guard<std::mutex> lock_notifier(m_mutex_notifier);
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

#ifdef DYNAMIC_OVERRIDE
    std::string getOverridePath() {
        std::ifstream file(TOSTRING(DYNAMIC_OVERRIDE));

        if (!file.is_open()) {
            return "";
        }

        std::string content;
        std::string line;

        while (std::getline(file, line)) {
            content += line;
        }

        file.close();

        content.erase(
            std::remove_if(content.begin(), content.end(),
                [](unsigned char c) { return !std::isalnum(c); }),
            content.end()
        );

        return "/etc/system_" + content + ".conf";
    }
#endif

    void refreshAll()
    {
        std::map<SettingKey, SettingValue> cust_data;
        parseConf("/etc/" + m_base + ".conf", cust_data, false);
#ifdef DYNAMIC_OVERRIDE
        parseConf(getOverridePath(), cust_data, false);
#endif
        parseConf("/var/" + m_base + ".conf", cust_data, true);
        refreshData(cust_data);
    }
};

GlobalSettings::GlobalSettings()
    : GlobalSettings("system")
{
}

GlobalSettings::GlobalSettings(const std::string &base)
    : m_ptr(std::make_unique<GlobalSettingsPrivate>(this, base))
{
    m_ptr->refreshAll();
}

SettingValue GlobalSettings::get(const std::string &key, const std::string &category, SettingValue def) const
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);
    std::string gkey = key;
    std::string gcategory = category;
    if(gcategory.empty() && gkey.starts_with("["))
    {
        size_t closing_bracket = gkey.find(']');
        if (closing_bracket == std::string::npos) {
            std::cerr << "ERROR: Malformed key (missing ']'): " << gkey << std::endl;
            return def;
        }
        gcategory = gkey.substr(1, closing_bracket - 1);
        gkey = gkey.substr(closing_bracket + 1);
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
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);
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

    // Validate ID to prevent path traversal attacks
    if (!isValidId(id)) {
        std::cerr << "ERROR: Invalid ID (potential path traversal): \"" << id << "\"" << std::endl;
        std::cerr << "       IDs must contain only alphanumeric characters, dash, and underscore" << std::endl;
        return false;
    }

    // Permission is OK, get old custom variable
    std::map<SettingKey, SettingValue> cust_data;
    m_ptr->parseFile("/var/" + m_ptr->m_base + ".conf.d/"+id+".conf", cust_data);
    cust_data[pkey] = value;

    // Use atomic write pattern: write to temp file, then rename
    std::string config_path = "/var/" + m_ptr->m_base + ".conf.d/" + id + ".conf";
    std::string temp_path = config_path + ".tmp." + std::to_string(getpid());

    // Write to temporary file
    std::ofstream file(temp_path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open temp file for writing: " << temp_path << std::endl;
        return false;
    }

    // Write all data with per-line error checking
    std::string last_category = "";
    for(const auto& [key, value] : cust_data) {
        if(key.first != last_category)
        {
            last_category = key.first.empty() ? "General" : key.first;
            if (!(file << "[" << last_category << "]\n")) {
                std::cerr << "ERROR: Write failed at category: " << last_category << std::endl;
                file.close();
                std::remove(temp_path.c_str());  // Clean up temp file
                return false;
            }
        }
        // Write owner marker if this key has an owner
        if (m_ptr->m_owner.count(key)) {
            if (!(file << "@" << key.second << " = " << m_ptr->m_owner[key] << "\n")) {
                std::cerr << "ERROR: Write failed at owner marker for key: " << key.second << std::endl;
                file.close();
                std::remove(temp_path.c_str());  // Clean up temp file
                return false;
            }
        }
        if (!(file << key.second << " = " << valueToStr(value) << "\n")) {
            std::cerr << "ERROR: Write failed at key: " << key.second << std::endl;
            file.close();
            std::remove(temp_path.c_str());  // Clean up temp file
            return false;
        }
    }

    // Flush and verify before closing
    file.flush();
    if (!file.good()) {
        std::cerr << "ERROR: Flush failed for temp file: " << temp_path << std::endl;
        file.close();
        std::remove(temp_path.c_str());
        return false;
    }

    file.close();
    if (!file) {
        std::cerr << "ERROR: Close failed for temp file: " << temp_path << std::endl;
        std::remove(temp_path.c_str());
        return false;
    }

    // Atomic rename - this is the critical operation
    if (std::rename(temp_path.c_str(), config_path.c_str()) != 0) {
        std::cerr << "ERROR: Failed to rename temp file to config file: " << strerror(errno) << std::endl;
        std::remove(temp_path.c_str());
        return false;
    }

    // Only NOW update memory (after successful atomic write)
    m_ptr->m_data[pkey] = value;
    return true;
}

std::string GlobalSettings::id() const
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);
    return m_ptr->m_id;
}

void GlobalSettings::setId(const std::string &newid)
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);

    // Validate ID to prevent path traversal attacks
    if (!isValidId(newid)) {
        std::cerr << "ERROR: Invalid ID (potential path traversal): \"" << newid << "\"" << std::endl;
        std::cerr << "       IDs must contain only alphanumeric characters, dash, and underscore" << std::endl;
        std::cerr << "       Keeping current ID: \"" << m_ptr->m_id << "\"" << std::endl;
        return;
    }

    m_ptr->m_id = newid;
}

void GlobalSettings::setNotifier(const std::function<void (const std::string &, const std::string &, SettingValue)> &callback)
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_notifier);
    m_ptr->m_notifier.push_back(callback);
    m_ptr->m_watcher.setEnable(true);
}

std::size_t GlobalSettings::size() const
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);
    return m_ptr->m_data.size();
}

void GlobalSettings::forEach(const std::function<void (const SettingKey &, const SettingValue &)> &callback) const
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_data);
    for (const auto& [key, value] : m_ptr->m_data) {
        callback(key, value);
    }
}

GlobalSettings::~GlobalSettings()
{
    m_ptr->m_watcher.setEnable(false);
}
