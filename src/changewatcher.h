/**
 * @file changewatcher.h
 * @brief File system change monitoring using inotify
 * @author GlobalSettings Team
 * @version 1.1.0
 *
 * This file provides the ChangeWatcher class which monitors files and directories
 * for changes using Linux inotify API. It runs a background thread that watches
 * for file modifications, additions, and deletions.
 *
 * @section usage Usage Example
 * @code
 * #include "changewatcher.h"
 *
 * ChangeWatcher watcher;
 * watcher.setFileWrited([](const std::string& path) {
 *     std::cout << "File modified: " << path << std::endl;
 * });
 * watcher.watch("/etc/myapp.conf");
 * watcher.setEnable(true);
 * @endcode
 *
 * @note Linux-specific (uses inotify)
 * @note Automatically starts a background thread for monitoring
 * @note All callbacks are invoked from the background thread
 */

#ifndef CHANGEWATCHER_H
#define CHANGEWATCHER_H

#include <functional>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <vector>

/**
 * @brief File system change monitoring class
 *
 * This class monitors files and directories for changes using Linux inotify.
 * It provides callbacks for three types of events:
 * - File written (IN_MODIFY, IN_CLOSE_WRITE)
 * - File added (IN_CREATE)
 * - File removed (IN_DELETE, IN_DELETE_SELF, IN_MOVED_FROM)
 *
 * The watcher runs in a separate thread and invokes callbacks when events occur.
 *
 * @note Thread-safe: Can be called from multiple threads
 * @note Uses inotify: Linux-specific implementation
 * @note Automatically manages inotify watch descriptors
 * @note RAII: Thread and file descriptors cleaned up in destructor
 *
 * @warning Callbacks are invoked from the background thread
 * @warning Keep callbacks fast to avoid blocking event processing
 */
class ChangeWatcher
{
    int m_fdnotify = -1;  ///< inotify file descriptor (-1 if not initialized)
    int m_fdevent = -1;   ///< Event file descriptor for thread signaling (-1 if not initialized)
    bool m_enabled {false};  ///< Whether watching is currently enabled
    std::thread m_thread;  ///< Background thread for inotify event processing

    mutable std::mutex m_mutex_state;  ///< Mutex protecting shared state

    std::vector<std::string> m_watches;  ///< List of watched paths
    std::map<int, std::string> m_wd;  ///< Map of watch descriptors to paths

    std::function<void (const std::string &)> m_filewrited {};  ///< Callback for file write events
    std::function<void (const std::string &)> m_fileadded;  ///< Callback for file add events
    std::function<void (const std::string &)> m_fileremoved;  ///< Callback for file remove events

public:
    /**
     * @brief Constructs a new ChangeWatcher instance
     *
     * Initializes inotify and creates the background monitoring thread.
     * The watcher starts in disabled state and must be enabled with setEnable().
     *
     * @throws std::runtime_error if inotify initialization fails
     */
    explicit ChangeWatcher();

    /**
     * @brief Destructor
     *
     * Stops the monitoring thread, closes all file descriptors, and releases resources.
     * Thread-safe shutdown is performed automatically.
     */
    virtual ~ChangeWatcher();

    /**
     * @brief Adds a path to the watch list
     *
     * Begins monitoring the specified file or directory for changes. If the path
     * is a directory, monitors all files within it.
     *
     * @param newPath Path to file or directory to monitor
     *
     * @note Thread-safe
     * @note Can be called multiple times to watch multiple paths
     * @note If path doesn't exist, watch is added but won't trigger events
     * @note For directories, watches all files but not subdirectories
     */
    void watch(const std::string &newPath);

    /**
     * @brief Enables or disables file watching
     *
     * Controls whether the watcher actively monitors for changes. When disabled,
     * no callbacks will be invoked even if files change.
     *
     * @param newVal true to enable watching, false to disable
     *
     * @note Thread-safe
     * @note Disabling doesn't remove watches, just stops processing events
     */
    void setEnable(bool newVal);

    /**
     * @brief Checks if watching is enabled
     *
     * @return true if currently monitoring for changes, false otherwise
     *
     * @note Thread-safe
     */
    bool enabled() const;

    /**
     * @brief Sets callback for file write events
     *
     * Registers a callback to be invoked when a watched file is modified.
     * Triggers on IN_MODIFY and IN_CLOSE_WRITE inotify events.
     *
     * @param callback Function to call when files are written
     *                 Parameter: full path to the modified file
     *
     * @note Thread-safe
     * @note Callback is invoked from the background thread
     * @note Keep callback fast to avoid blocking event processing
     *
     * @warning Only one callback can be registered (replaces previous)
     */
    void setFileWrited(const std::function<void(const std::string&)> &callback);

    /**
     * @brief Sets callback for file add events
     *
     * Registers a callback to be invoked when a file is created in a watched directory.
     * Triggers on IN_CREATE inotify events.
     *
     * @param callback Function to call when files are added
     *                 Parameter: full path to the new file
     *
     * @note Thread-safe
     * @note Callback is invoked from the background thread
     * @note Keep callback fast to avoid blocking event processing
     *
     * @warning Only one callback can be registered (replaces previous)
     */
    void setFileAdded(const std::function<void(const std::string&)> &callback);

    /**
     * @brief Sets callback for file remove events
     *
     * Registers a callback to be invoked when a watched file is deleted or moved.
     * Triggers on IN_DELETE, IN_DELETE_SELF, and IN_MOVED_FROM inotify events.
     *
     * @param callback Function to call when files are removed
     *                 Parameter: full path to the removed file
     *
     * @note Thread-safe
     * @note Callback is invoked from the background thread
     * @note Keep callback fast to avoid blocking event processing
     *
     * @warning Only one callback can be registered (replaces previous)
     */
    void setFileRemoved(const std::function<void(const std::string&)> &callback);

private:
    /**
     * @brief Background thread main loop
     *
     * Runs continuously, blocking on select() waiting for inotify events.
     * Processes events and invokes appropriate callbacks.
     *
     * @param m Pointer to the ChangeWatcher instance
     *
     * @note Runs in separate thread
     * @note Exits when destructor signals via m_fdevent
     */
    static void _runningLoop(ChangeWatcher *m);

    /**
     * @brief Internal method to add an inotify watch
     *
     * Adds an inotify watch descriptor for the specified path.
     *
     * @param newPath Path to add to inotify
     *
     * @note Not thread-safe - caller must hold m_mutex_state
     * @note Handles both files and directories
     */
    void _addWatch(const std::string& newPath);
};

#endif // CHANGEWATCHER_H
