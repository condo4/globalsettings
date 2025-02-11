#include "changewatcher.h"
#include <iostream>
#include <filesystem>
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
        for(auto &newPath: m_watches)
        {
            _addWatch(newPath);
        }
        m_fdevent = eventfd(0, 0);
        m_enabled = true;
        m_thread =  std::thread(_runningLoop, this);
    }
    else
    {
        // Stop thread
        m_enabled = false;
        uint64_t value = 1;
        write(m_fdevent, &value, sizeof(value));
        close(m_fdevent);
        m_thread.join();
        close(m_fdnotify);
    }
}

bool ChangeWatcher::enabled() const
{
    return m_enabled;
}

void ChangeWatcher::setFileWrited(const std::function<void (const std::string &)> &callback)
{
    m_filewrited = callback;
}

void ChangeWatcher::setFileAdded(const std::function<void (const std::string &)> &callback)
{
    m_fileadded = callback;
}

void ChangeWatcher::setFileRemoved(const std::function<void (const std::string &)> &callback)
{
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
                        if((event->mask & IN_CLOSE_WRITE) && (m->m_filewrited != nullptr))
                        {
                            m->m_filewrited(m->m_wd[event->wd]);
                        }
                        if((event->mask & IN_CREATE) && (m->m_fileadded != nullptr))
                        {
                            m->m_fileadded(m->m_wd[event->wd]);
                        }
                        if((event->mask & IN_DELETE) && (m->m_fileremoved != nullptr))
                        {
                            m->m_fileremoved(m->m_wd[event->wd]);
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
    if(wd > -1)  m_wd[wd] = newPath;
}
