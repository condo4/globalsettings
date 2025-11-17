#ifndef CHANGEWATCHER_H
#define CHANGEWATCHER_H

#include <functional>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <vector>

class ChangeWatcher
{
    int m_fdnotify = -1;  // Initialize to -1 (invalid FD)
    int m_fdevent = -1;   // Initialize to -1 (invalid FD)
    bool m_enabled {false};
    std::thread m_thread;

    // Mutex to protect shared data accessed by multiple threads
    mutable std::mutex m_mutex_state;

    std::vector<std::string> m_watches;
    std::map<int, std::string> m_wd;

    std::function<void (const std::string &)> m_filewrited {};
    std::function<void (const std::string &)> m_fileadded;
    std::function<void (const std::string &)> m_fileremoved;

public:
    explicit ChangeWatcher();
    virtual ~ChangeWatcher();
    void watch(const std::string &newPath);

    void setEnable(bool newVal);
    bool enabled() const;

    void setFileWrited(const std::function<void(const std::string&)> &callback);
    void setFileAdded(const std::function<void(const std::string&)> &callback);
    void setFileRemoved(const std::function<void(const std::string&)> &callback);

private:
    static void _runningLoop(ChangeWatcher *m);
    void _addWatch(const std::string& newPath);
};

#endif // CHANGEWATCHER_H
