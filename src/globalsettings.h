#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <functional>
#include <memory>
#include <variant>
#include <string>

/** @brief Key type for settings, composed of category and name */
using SettingKey = std::pair<std::string, std::string>;

/** @brief Value type that can hold string, int, double or boolean */
using SettingValue = std::variant<std::string, int, double, bool>;

class GlobalSettingsPrivate;

/**
 * @brief Main class for managing global settings
 *
 * This class provides a thread-safe interface for storing and retrieving
 * configuration settings with support for different value types.
 */
class GlobalSettings
{
public:
    /** @brief Constructs a new GlobalSettings instance */
    explicit GlobalSettings();

    /** @brief Virtual destructor */
    virtual ~GlobalSettings();

    /**
     * @brief Retrieves a setting value
     * @param key The setting key
     * @param category The setting category (optional)
     * @param def Default value if setting not found (optional)
     * @return The setting value or default if not found
     */
    SettingValue get(const std::string &key, const std::string &category = "", SettingValue def = SettingValue()) const;

    /**
     * @brief Sets a setting value
     * @param key The setting key
     * @param value The value to set
     * @param category The setting category (optional)
     * @return true if successful, false otherwise
     */
    bool set(const std::string &key, SettingValue value, const std::string &category = "");

    /**
     * @brief Gets the current instance identifier
     * @return The instance ID string, by default, this is the excecutable filename
     */
    std::string id() const;

    /**
     * @brief Sets the instance identifier
     * @param newid The new ID to set
     */
    void setId(const std::string &newid);

    /**
     * @brief Sets a callback for setting changes notification
     * @param callback Function to call when settings change
     */
    void setNotifier(const std::function<void(const std::string&, const std::string&, SettingValue)>& callback);

    /**
     * @brief Gets the number of settings
     * @return The total count of settings
     */
    std::size_t size() const;

    /**
     * @brief Iterates over all settings in a thread-safe manner
     * @param callback Function called for each key-value pair
     */
    void forEach(const std::function<void(const SettingKey&, const SettingValue&)>& callback) const;

private:
    std::unique_ptr<GlobalSettingsPrivate> m_ptr; ///< Private implementation pointer
};

#endif // GLOBALSETTINGS_H
