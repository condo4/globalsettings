#include "changewatcher.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <unistd.h>

ChangeWatcher::ChangeWatcher()
{
}

ChangeWatcher::~ChangeWatcher()
{
    setEnable(false);
}

void ChangeWatcher::watch(const std::string &newPath)
{
    std::lock_guard<std::mutex> lock(m_mutex_state);
    if(std::find(m_watches.begin(), m_watches.end(), newPath) == m_watches.end())
    {
        m_watches.push_back(newPath);
        if(m_enabled)
        {
            _addWatch(newPath);
        }
    }
}

void ChangeWatcher::setEnable(bool newVal)
{
    if(m_enabled == newVal)
        return;

    if(newVal)
    {
        // Start thread
        m_fdnotify = inotify_init();
        if (m_fdnotify == -1) {
            std::cerr << "ERROR: inotify_init() failed" << std::endl;
            return;
        }

        // Copy watches vector while holding lock to avoid race condition
        std::vector<std::string> watches_copy;
        {
            std::lock_guard<std::mutex> lock(m_mutex_state);
            watches_copy = m_watches;
        }

        // Now iterate the copy without holding the lock
        for(auto &newPath: watches_copy)
        {
            std::lock_guard<std::mutex> lock(m_mutex_state);
            _addWatch(newPath);
        }

        m_fdevent = eventfd(0, 0);
        if (m_fdevent == -1) {
            std::cerr << "ERROR: eventfd() failed" << std::endl;
            close(m_fdnotify);
            m_fdnotify = -1;
            return;
        }

        m_enabled = true;
        m_thread =  std::thread(_runningLoop, this);
    }
    else
    {
        // Stop thread
        m_enabled = false;

        // Signal thread to stop (only if eventfd was successfully created)
        if (m_fdevent != -1) {
            uint64_t value = 1;
            write(m_fdevent, &value, sizeof(value));
            close(m_fdevent);
            m_fdevent = -1;
        }

        // Wait for thread to finish (only if thread is joinable)
        if (m_thread.joinable()) {
            m_thread.join();
        }

        // Close inotify fd (only if it was successfully created)
        if (m_fdnotify != -1) {
            close(m_fdnotify);
            m_fdnotify = -1;
        }
    }
}

bool ChangeWatcher::enabled() const
{
    return m_enabled;
}

void ChangeWatcher::setFileWrited(const std::function<void (const std::string &)> &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex_state);
    m_filewrited = callback;
}

void ChangeWatcher::setFileAdded(const std::function<void (const std::string &)> &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex_state);
    m_fileadded = callback;
}

void ChangeWatcher::setFileRemoved(const std::function<void (const std::string &)> &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex_state);
    m_fileremoved = callback;
}

void ChangeWatcher::_runningLoop(ChangeWatcher *m)
{
    const size_t event_size = sizeof(struct inotify_event);
    const size_t buffer_size = 1024 * (event_size + 16);
    char buffer[buffer_size];

    while (m->m_enabled) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m->m_fdevent, &rfds);
        FD_SET(m->m_fdnotify, &rfds);
        int nfds = std::max(m->m_fdevent, m->m_fdnotify) + 1;

        if (select(nfds, &rfds, nullptr, nullptr, nullptr) > 0) {
            if (FD_ISSET(m->m_fdevent, &rfds)) {
                break; // Signal d'arrêt reçu
            }
            if (FD_ISSET(m->m_fdnotify, &rfds)) {
                int length = read(m->m_fdnotify, buffer, buffer_size);
                if (length > 0) {
                    size_t i = 0;
                    while (i < length) {
                        auto* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);

                        // Copy callback and path while holding lock, then call without lock
                        std::string path;
                        std::function<void(const std::string&)> callback;

                        {
                            std::lock_guard<std::mutex> lock(m->m_mutex_state);
                            auto it = m->m_wd.find(event->wd);
                            if (it != m->m_wd.end()) {
                                path = it->second;

                                if((event->mask & IN_CLOSE_WRITE) && (m->m_filewrited != nullptr))
                                {
                                    callback = m->m_filewrited;
                                }
                                else if((event->mask & IN_CREATE) && (m->m_fileadded != nullptr))
                                {
                                    callback = m->m_fileadded;
                                }
                                else if((event->mask & IN_DELETE) && (m->m_fileremoved != nullptr))
                                {
                                    callback = m->m_fileremoved;
                                }
                            }
                        }

                        // Call callback without holding lock to prevent deadlock
                        if (callback && !path.empty()) {
                            callback(path);
                        }

                        i += event_size + event->len;
                    }
                }
            }
        }
    }
}

void ChangeWatcher::_addWatch(const std::string &newPath)
{
    int wd = -1;
    if(std::filesystem::is_directory(newPath))
    {
        wd = inotify_add_watch(m_fdnotify, newPath.c_str(), IN_CREATE | IN_DELETE);
    }
    else
    {
        wd = inotify_add_watch(m_fdnotify, newPath.c_str(), IN_CLOSE_WRITE);
    }

    if(wd == -1) {
        std::cerr << "WARNING: Cannot watch path: " << newPath << std::endl;
    } else {
        m_wd[wd] = newPath;
    }
}
