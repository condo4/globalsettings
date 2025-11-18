/**
 * @file globalsettings.h
 * @brief Thread-safe global configuration settings manager
 * @author GlobalSettings Team
 * @version 1.1.0
 *
 * This file provides the main interface for the GlobalSettings library, which manages
 * configuration settings from multiple sources (/etc and /var) with thread-safe access,
 * automatic file watching, and change notifications.
 *
 * @section usage Usage Example
 * @code
 * #include "globalsettings.h"
 *
 * - Create settings instance
 * GlobalSettings settings;
 *
 * - Read a value
 * auto value = settings.get("hostname", "Network", SettingValue("localhost"));
 * std::string hostname = std::get<std::string>(value);
 *
 * - Write a value (requires ownership)
 * settings.set("my_setting", SettingValue(42), "MyApp");
 *
 * - Listen for changes
 * settings.setNotifier([](const std::string& cat, const std::string& key, SettingValue val) {
 *     std::cout << "Setting changed: " << cat << ">" << key << std::endl;
 * });
 * @endcode
 *
 * @section thread_safety Thread Safety
 * All public methods are thread-safe and can be called from multiple threads concurrently.
 * Internal mutexes protect all shared data structures.
 *
 * @section file_format Configuration File Format
 * Configuration files use INI-style format:
 * @code
 * [Category]
 * key = value
 * @owner_key = application_id  # Ownership marker
 * @endcode
 */

#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <functional>
#include <variant>
#include <string>
#include <memory>

/**
 * @brief Key type for settings, composed of category and name
 *
 * The first element is the category (empty string for "General" category),
 * and the second element is the setting name.
 */
using SettingKey = std::pair<std::string, std::string>;

/**
 * @brief Value type that can hold string, int, double or boolean
 *
 * Settings values are automatically parsed from configuration files:
 * - Numeric strings become int or double
 * - "true"/"false" become bool
 * - Everything else remains string
 */
using SettingValue = std::variant<std::string, int, double, bool>;

class GlobalSettingsPrivate;

/**
 * @brief Main class for managing global settings
 *
 * This class provides a thread-safe interface for storing and retrieving
 * configuration settings with support for different value types. It reads
 * configuration from multiple sources in priority order:
 * 1. /etc/{base}.conf (system-wide, read-only)
 * 2. /etc/{base}.conf.d/<fragment>.conf (system-wide fragments, read-only)
 * 3. /var/{base}.conf.d/<appid>.conf (per-application, writable)
 *
 * Settings can only be modified if the application owns them (marked with
 * @key = appid in configuration files).
 *
 * @note All methods are thread-safe
 * @note Automatically watches configuration files for changes
 * @note Uses RAII - files are watched from construction until destruction
 */
class GlobalSettings
{
public:
    /**
     * @brief Constructs a new GlobalSettings instance with default base name "system"
     *
     * Creates a settings manager that reads from:
     * - /etc/system.conf
     * - /etc/system.conf.d/<fragment>.conf
     * - /var/system.conf.d/<appid>.conf
     *
     * The application ID is automatically determined from the executable name.
     *
     * @throws std::runtime_error if file watching initialization fails
     */
    explicit GlobalSettings();

    /**
     * @brief Constructs a new GlobalSettings instance with custom base name
     * @param base The base name for configuration files (e.g., "fiber_application")
     *
     * The base name determines the configuration file paths:
     * - /etc/{base}.conf
     * - /etc/{base}.conf.d/ (all .conf files)
     * - /var/{base}.conf.d/<appid>.conf
     *
     * Default base is "system" if using the parameterless constructor.
     *
     * @throws std::runtime_error if file watching initialization fails
     */
    explicit GlobalSettings(const std::string &base);

    /**
     * @brief Virtual destructor
     *
     * Stops file watching and releases all resources.
     * Thread-safe cleanup is performed automatically.
     */
    virtual ~GlobalSettings();

    /**
     * @brief Retrieves a setting value
     *
     * Looks up a setting by key and category. If not found, returns the default value.
     * The category can be specified explicitly or embedded in the key using [Category]key syntax.
     *
     * @param key The setting key name (can include [Category] prefix)
     * @param category The setting category (empty string for "General" category)
     * @param def Default value if setting not found
     * @return The setting value or default if not found
     *
     * @note Thread-safe - can be called concurrently from multiple threads
     * @note Returns a copy of the value, not a reference
     *
     * Example:
     * @code
     * auto val1 = settings.get("hostname", "Network", SettingValue("localhost"));
     * auto val2 = settings.get("[Network]hostname", "", SettingValue("localhost"));  // Equivalent
     * std::string host = std::get<std::string>(val1);
     * @endcode
     */
    SettingValue get(const std::string &key, const std::string &category = "", SettingValue def = SettingValue()) const;

    /**
     * @brief Sets a setting value
     *
     * Writes a setting value to the configuration file. The application must own
     * the setting (marked with @key = appid) for the write to succeed.
     *
     * @param key The setting key name
     * @param value The value to set (string, int, double, or bool)
     * @param category The setting category (empty string for "General")
     * @return true if successful, false if not owned or write failed
     *
     * @note Thread-safe - uses atomic file writes with rename()
     * @note Only owned settings can be modified (enforced by ownership markers)
     * @note Validates ID for path traversal attacks
     * @note Triggers change notifications after successful write
     *
     * Example:
     * @code
     * if (settings.set("port", SettingValue(8080), "Network")) {
     *     std::cout << "Port updated successfully" << std::endl;
     * }
     * @endcode
     */
    bool set(const std::string &key, SettingValue value, const std::string &category = "");

    /**
     * @brief Gets the current instance identifier
     *
     * Returns the application ID used for determining which settings this
     * instance can modify. By default, this is extracted from the executable
     * filename (/proc/self/exe).
     *
     * @return The instance ID string (executable name without path/extension)
     *
     * @note Thread-safe
     * @note Used for ownership checks when writing settings
     */
    std::string id() const;

    /**
     * @brief Sets the instance identifier
     *
     * Changes the application ID used for ownership checks. This affects which
     * settings can be modified by set(). The ID is validated to prevent path
     * traversal attacks (only alphanumeric, dash, and underscore allowed).
     *
     * @param newid The new ID to set (must match pattern: [a-zA-Z0-9_-]+)
     *
     * @note Thread-safe
     * @note Rejects invalid IDs (prints error but doesn't throw)
     * @note Use with caution - changing ID affects write permissions
     *
     * @warning Only set this if you need to assume a different identity
     */
    void setId(const std::string &newid);

    /**
     * @brief Sets a callback for setting changes notification
     *
     * Registers a callback function that will be invoked whenever a setting
     * changes (either from file watching or manual set() calls).
     *
     * @param callback Function to call when settings change
     *                 Parameters: (category, key, new_value)
     * @return Unique notifier ID that can be used to remove the notifier later
     *
     * @note Thread-safe
     * @note Callback is invoked with mutex held - keep it fast!
     * @note Multiple notifiers can be registered (they're additive)
     * @note Callback is called from the file watcher thread
     * @note You must call removeNotifier() with the returned ID to unregister
     *
     * Example:
     * @code
     * size_t id = settings.setNotifier([](const std::string& cat, const std::string& key, SettingValue val) {
     *     std::cout << "Changed: " << cat << ">" << key << " = ";
     *     std::visit([](auto&& v) { std::cout << v; }, val);
     *     std::cout << std::endl;
     * });
     * // Later: settings.removeNotifier(id);
     * @endcode
     */
    std::size_t setNotifier(const std::function<void(const std::string&, const std::string&, SettingValue)>& callback);

    /**
     * @brief Removes a previously registered notifier
     *
     * Unregisters a callback that was previously registered with setNotifier().
     * After this call, the callback will no longer be invoked for setting changes.
     *
     * @param notifierId The ID returned by setNotifier() when the callback was registered
     *
     * @note Thread-safe
     * @note Safe to call with invalid IDs (no-op if ID not found)
     * @note Must be called before the callback's captured variables are destroyed
     *
     * Example:
     * @code
     * size_t id = settings.setNotifier([](auto&, auto&, auto) { ... });
     * // ... later ...
     * settings.removeNotifier(id);
     * @endcode
     */
    void removeNotifier(std::size_t notifierId);

    /**
     * @brief Gets the number of settings
     *
     * Returns the total count of all loaded settings across all categories
     * and configuration files.
     *
     * @return The total count of settings
     *
     * @note Thread-safe
     * @note Returns size at time of call (may change if files are modified)
     */
    std::size_t size() const;

    /**
     * @brief Iterates over all settings in a thread-safe manner
     *
     * Calls the provided callback function once for each setting. The iteration
     * is performed with a lock held, ensuring a consistent snapshot of the data.
     *
     * @param callback Function called for each key-value pair
     *                 Parameters: (SettingKey, SettingValue)
     *
     * @note Thread-safe - entire iteration is protected by mutex
     * @note Callback is invoked with mutex held - keep it fast!
     * @note Do not call other GlobalSettings methods from the callback (deadlock risk)
     *
     * Example:
     * @code
     * settings.forEach([](const SettingKey& key, const SettingValue& val) {
     *     std::cout << key.first << ">" << key.second << " = ";
     *     std::visit([](auto&& v) { std::cout << v; }, val);
     *     std::cout << std::endl;
     * });
     * @endcode
     */
    void forEach(const std::function<void(const SettingKey&, const SettingValue&)>& callback) const;

private:
    std::unique_ptr<GlobalSettingsPrivate> m_ptr; ///< Private implementation pointer
};

#endif // GLOBALSETTINGS_H
