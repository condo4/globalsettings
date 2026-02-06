/**
 * @file globalsettings.cpp
 * @brief Implementation of GlobalSettings class
 * @author GlobalSettings Team
 * @version 1.1.0
 *
 * This file contains the implementation of the GlobalSettings class and its
 * private implementation (GlobalSettingsPrivate). It handles:
 * - Configuration file parsing (INI-style format)
 * - Multi-source configuration merging (/etc, /var and /tmp)
 * - Thread-safe access to settings
 * - Atomic file writes for setting modifications
 * - File watching for automatic updates
 * - Ownership-based write permission enforcement
 *
 * @section architecture Architecture
 * Uses the PIMPL (Pointer to Implementation) idiom to hide implementation details
 * and maintain ABI stability. The GlobalSettingsPrivate class contains:
 * - Data storage (map of settings)
 * - File watcher for automatic updates
 * - Ownership information for write permissions
 * - Mutexes for thread safety
 * - File parsing and writing logic
 *
 * @section thread_safety Thread Safety
 * All operations are protected by mutexes:
 * - m_mutex_data: Protects settings data and ownership maps
 * - m_mutex_notifier: Protects notifier callback vector
 *
 * @section file_format Configuration File Format
 * INI-style format with ownership markers:
 * @code
 * [Category]
 * key = value
 * @owner_key = application_id  # Marks who can modify this key (saved in /var/)
 * ~volatile_key = application_id  # Marks volatile variable (saved in /tmp/)
 * @endcode
 *
 * @note Uses inotify for file watching on Linux
 * @note Implements atomic writes using temp file + rename pattern
 * @note Validates IDs to prevent path traversal attacks
 */

#include "globalsettings.h"
#include <iostream>
#include <fstream>
#include <map>
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
    std::map<SettingKey, SettingValue> m_data;
    ChangeWatcher m_watcher;
    std::map<SettingKey, std::string> m_owner;
    std::map<SettingKey, std::string> m_volatile_owner;
    std::string m_id;
    std::string m_base;
    std::mutex m_mutex_data;
    std::mutex m_mutex_notifier;
    bool m_bypass {false};
    std::map<std::size_t, std::function<void(const std::string&, const std::string&, SettingValue)>> m_notifier;
    std::size_t m_nextNotifierId {0};

public:
    GlobalSettingsPrivate(const std::string &base)
        : m_base(base) {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count <= 0 || count >= PATH_MAX) {
            std::cerr << "WARNING: Cannot determine application path, using 'unknown' as ID" << std::endl;
            m_id = "";
        } else {
            std::string binary = std::string(result, count);
            std::filesystem::path bin = binary;
            m_id = bin.filename();

            if(m_id == "settings")
            {
                // Only "settings cli tools can bypass the owner protection"
                m_bypass = true;
            }
        }

        m_watcher.setFileAdded([this](const std::string path){ refreshDirectory(path); });
        m_watcher.setFileRemoved([this](const std::string /*path*/){ refreshAll(); });
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
            // Check for += operator
            size_t pos = line.find("+=");
            bool is_append = false;
            if (pos != std::string::npos) {
                is_append = true;
            } else {
                pos = line.find("=");
                if (pos == std::string::npos) {
                    std::cerr << "WARNING: Malformed config line (missing '='): " << line << std::endl;
                    continue;
                }
            }

            std::string key = line.substr(0, pos);
            trim(key);
            std::string value = line.substr(pos + (is_append ? 2 : 1));
            trim(value);

            if(key.starts_with("@"))
            {
                m_owner[SettingKey(category, key.substr(1))] = value;
                continue;
            }
            if(key.starts_with("~"))
            {
                m_volatile_owner[SettingKey(category, key.substr(1))] = value;
                continue;
            }
            if(key.ends_with("/owner"))
            {
                std::cerr << "WARNING: Legacy usage for owner: " << key << " should be @" << key.substr(0, key.size() - 6) << std::endl;
                m_owner[SettingKey(category, key.substr(0, key.size() - 6))] = value;
                continue;
            }

            SettingKey skey(category, key);
            SettingValue new_value = makeValue(value);

            if (is_append && data.count(skey)) {
                // Get current value
                SettingValue current = data[skey];

                // Handle append based on type
                if (std::holds_alternative<std::string>(current) && std::holds_alternative<std::string>(new_value)) {
                    // String concatenation with space handling
                    std::string current_str = std::get<std::string>(current);
                    std::string new_str = std::get<std::string>(new_value);

                    // Ensure exactly one space between values
                    bool current_has_trailing_space = !current_str.empty() && current_str.back() == ' ';
                    bool new_has_leading_space = !new_str.empty() && new_str.front() == ' ';

                    if (current_has_trailing_space || new_has_leading_space) {
                        // Trim spaces from both ends
                        while (!current_str.empty() && current_str.back() == ' ') {
                            current_str.pop_back();
                        }
                        while (!new_str.empty() && new_str.front() == ' ') {
                            new_str.erase(0, 1);
                        }
                    }

                    // Add exactly one space and concatenate
                    data[skey] = SettingValue(current_str + " " + new_str);
                }
                else if (std::holds_alternative<int>(current) && std::holds_alternative<int>(new_value)) {
                    // Integer addition
                    data[skey] = SettingValue(std::get<int>(current) + std::get<int>(new_value));
                }
                else if (std::holds_alternative<double>(current) && std::holds_alternative<double>(new_value)) {
                    // Double addition
                    data[skey] = SettingValue(std::get<double>(current) + std::get<double>(new_value));
                }
                else if (std::holds_alternative<int>(current) && std::holds_alternative<double>(new_value)) {
                    // Mixed int + double = double
                    data[skey] = SettingValue(std::get<int>(current) + std::get<double>(new_value));
                }
                else if (std::holds_alternative<double>(current) && std::holds_alternative<int>(new_value)) {
                    // Mixed double + int = double
                    data[skey] = SettingValue(std::get<double>(current) + std::get<int>(new_value));
                }
                else {
                    std::cerr << "WARNING: Cannot append incompatible types for key: " << key << std::endl;
                    data[skey] = new_value;  // Fallback to replacement
                }
            } else {
                data[skey] = new_value;
            }
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
                for(auto& [id, notifier]: m_notifier)
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
        parseConf("/tmp/" + m_base + ".conf", cust_data, true);
        refreshData(cust_data);
    }
};

GlobalSettings::GlobalSettings()
    : GlobalSettings("system")
{
}

GlobalSettings::GlobalSettings(const std::string &base)
    : m_ptr(std::make_unique<GlobalSettingsPrivate>(base))
{
    m_ptr->refreshAll();
}

GlobalSettings::GlobalSettings(GlobalSettings&& other) noexcept
    : m_ptr(std::move(other.m_ptr))
{
}

GlobalSettings& GlobalSettings::operator=(GlobalSettings&& other) noexcept
{
    if (this != &other) {
        m_ptr = std::move(other.m_ptr);
    }
    return *this;
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
    bool is_volatile = false;

    // Check if variable is regular RW or volatile
    if(m_ptr->m_owner.count(pkey))
    {
        is_volatile = false;
    }
    else if(m_ptr->m_volatile_owner.count(pkey))
    {
        is_volatile = true;
    }
    else
    {
        std::cerr << "Variable " << category << ">" << key << " is not writable" << std::endl;
        return false;
    }

    if(m_ptr->m_bypass)
    {
        id = is_volatile ? m_ptr->m_volatile_owner[pkey] : m_ptr->m_owner[pkey];
    }
    else
    {
        id = m_ptr->m_id;
        std::string expected_owner = is_volatile ? m_ptr->m_volatile_owner[pkey] : m_ptr->m_owner[pkey];
        if(expected_owner != id)
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
    std::string base_path = is_volatile ? "/tmp/" : "/var/";
    m_ptr->parseFile(base_path + m_ptr->m_base + ".conf.d/"+id+".conf", cust_data);
    cust_data[pkey] = value;

    // Use atomic write pattern: write to temp file, then rename
    std::string config_path = base_path + m_ptr->m_base + ".conf.d/" + id + ".conf";
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
        // Write volatile owner marker if this key has a volatile owner
        if (m_ptr->m_volatile_owner.count(key)) {
            if (!(file << "~" << key.second << " = " << m_ptr->m_volatile_owner[key] << "\n")) {
                std::cerr << "ERROR: Write failed at volatile owner marker for key: " << key.second << std::endl;
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
    m_ptr->m_id = newid;
}

std::size_t GlobalSettings::setNotifier(const std::function<void (const std::string &, const std::string &, SettingValue)> &callback)
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_notifier);
    std::size_t id = m_ptr->m_nextNotifierId++;
    m_ptr->m_notifier[id] = callback;
    m_ptr->m_watcher.setEnable(true);
    return id;
}

void GlobalSettings::removeNotifier(std::size_t notifierId)
{
    std::lock_guard<std::mutex> lock(m_ptr->m_mutex_notifier);
    auto it = m_ptr->m_notifier.find(notifierId);
    if (it != m_ptr->m_notifier.end()) {
        m_ptr->m_notifier.erase(it);
    }

    // If no more notifiers, disable file watching
    if (m_ptr->m_notifier.empty()) {
        m_ptr->m_watcher.setEnable(false);
    }
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
    if (m_ptr) {
        m_ptr->m_watcher.setEnable(false);
    }
}
