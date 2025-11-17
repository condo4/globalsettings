# Thread Safety

## Overview

GlobalSettings is designed to be fully thread-safe, allowing concurrent access from multiple threads without external synchronization. This document explains the thread safety guarantees and best practices.

## Thread Safety Guarantees

### All Operations Are Thread-Safe

Every public method in GlobalSettings can be called safely from multiple threads:

```cpp
GlobalSettings settings;

// Thread 1
std::thread t1([&settings]() {
    auto value = settings.get("key1", "category");
});

// Thread 2
std::thread t2([&settings]() {
    settings.set("key2", SettingValue(42), "category");
});

// Thread 3
std::thread t3([&settings]() {
    settings.forEach([](const SettingKey& k, const SettingValue& v) {
        // Process each setting
    });
});

t1.join();
t2.join();
t3.join();
```

### Specific Guarantees

| Operation | Thread Safety | Notes |
|-----------|--------------|-------|
| `get()` | ✓ Thread-safe | Multiple threads can read simultaneously |
| `set()` | ✓ Thread-safe | Multiple threads can write simultaneously |
| `forEach()` | ✓ Thread-safe | Consistent snapshot during iteration |
| `setNotifier()` | ✓ Thread-safe | Callback registration is synchronized |
| `size()` | ✓ Thread-safe | Returns consistent count |
| `id()` / `setId()` | ✓ Thread-safe | Instance ID operations synchronized |

## Internal Synchronization

### Mutex Protection

GlobalSettings uses a single mutex to protect all shared state:

```cpp
class GlobalSettings {
private:
    mutable std::mutex mutex_;  // Protects all member data
    std::map<...> settings_;
    std::map<...> owners_;
    std::function<...> notifier_;
};
```

**Implementation pattern:**

```cpp
SettingValue GlobalSettings::get(...) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Access settings_ safely
    return settings_[...];
}

bool GlobalSettings::set(...) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Modify settings_ safely
    settings_[...] = value;
    return true;
}
```

### Lock Scope

Locks are held only for the duration of the operation:
- **Short critical sections**: Minimizes contention
- **No nested locking**: Eliminates deadlock risk
- **Exception-safe**: std::lock_guard ensures unlock on exceptions

## Common Patterns

### Pattern 1: Multiple Readers

Multiple threads reading different or same settings:

```cpp
GlobalSettings settings;

// Safe: Multiple concurrent reads
std::thread readers[10];
for (int i = 0; i < 10; i++) {
    readers[i] = std::thread([&settings, i]() {
        auto value = settings.get("key" + std::to_string(i));
        // Process value
    });
}

for (auto& t : readers) t.join();
```

**Performance:** Readers serialize on the mutex, but operations are fast.

### Pattern 2: Concurrent Read/Write

Threads reading and writing simultaneously:

```cpp
GlobalSettings settings;

// Reader thread
std::thread reader([&settings]() {
    while (running) {
        auto value = settings.get("status");
        processValue(value);
        std::this_thread::sleep_for(100ms);
    }
});

// Writer thread
std::thread writer([&settings]() {
    while (running) {
        settings.set("status", SettingValue("active"));
        std::this_thread::sleep_for(1s);
    }
});

reader.join();
writer.join();
```

**Guarantee:** Reads always see either old or new value, never partial/corrupted data.

### Pattern 3: Iteration with Concurrent Modifications

```cpp
GlobalSettings settings;

// Thread 1: Iterating
std::thread t1([&settings]() {
    settings.forEach([](const SettingKey& key, const SettingValue& value) {
        // Process each setting
        // Guaranteed consistent snapshot during iteration
    });
});

// Thread 2: Modifying
std::thread t2([&settings]() {
    settings.set("new_key", SettingValue(123));
});

t1.join();
t2.join();
```

**Guarantee:** `forEach()` sees a consistent snapshot. New settings from `t2` may or may not be visible, depending on timing.

### Pattern 4: Change Notifications

```cpp
GlobalSettings settings;

// Set up notifier (thread-safe)
settings.setNotifier([](const std::string& cat,
                        const std::string& key,
                        SettingValue value) {
    std::cout << "Changed: " << cat << ":" << key << std::endl;
    // Callback is synchronized
});

// Multiple threads can trigger notifications
std::thread t1([&settings]() {
    settings.set("key1", SettingValue(1));
});

std::thread t2([&settings]() {
    settings.set("key2", SettingValue(2));
});

t1.join();
t2.join();
```

**Guarantee:** Callbacks are invoked serially, one at a time.

## File Watcher Thread

### Background Monitoring

ChangeWatcher runs a separate thread for file monitoring:

```cpp
class ChangeWatcher {
private:
    std::thread watch_thread_;

    void monitorLoop() {
        while (running_) {
            // Read inotify events
            // Trigger configuration reload
            // Invoke callbacks
        }
    }
};
```

### Thread Coordination

```
Main Thread              Watcher Thread           Application Threads
    |                           |                         |
    |--- get() ---------->      |                         |
    |<-- value -----------      |                         |
    |                           |                         |
    |                           |--- file change          |
    |                           |--- reload config        |
    |                           |--- invoke callback ---->|
    |                           |                         |--- get() --->
    |                           |                         |<-- new value -
```

### Synchronization Points

1. **Configuration reload**: ChangeWatcher acquires GlobalSettings mutex
2. **Callback invocation**: Callbacks execute with mutex held
3. **Application access**: Application threads acquire same mutex

**Implication:** File changes and application access are fully synchronized.

## Performance Considerations

### Lock Contention

With coarse-grained locking, contention can occur under high load:

```cpp
// High contention scenario
for (int i = 0; i < 1000; i++) {
    threads[i] = std::thread([&settings]() {
        settings.get("key");  // All threads wait for mutex
    });
}
```

**Mitigation strategies:**
1. Cache frequently accessed settings
2. Batch operations when possible
3. Use local copies for read-heavy workloads

### Caching Pattern

```cpp
class ConfigCache {
private:
    GlobalSettings& settings_;
    std::string cached_host_;
    int cached_port_;
    std::chrono::steady_clock::time_point last_refresh_;

public:
    void refresh() {
        // Only access GlobalSettings during refresh
        cached_host_ = std::get<std::string>(
            settings_.get("host", "network", SettingValue("localhost"))
        );
        cached_port_ = std::get<int>(
            settings_.get("port", "network", SettingValue(8080))
        );
        last_refresh_ = std::chrono::steady_clock::now();
    }

    std::string host() const { return cached_host_; }
    int port() const { return cached_port_; }
};

// Use cache for high-frequency access
ConfigCache cache(settings);
cache.refresh();  // Infrequent

// Fast, lock-free access
std::string host = cache.host();
int port = cache.port();
```

## Best Practices

### 1. Don't Hold External Locks

```cpp
// Bad: Potential deadlock
std::mutex external_mutex;

{
    std::lock_guard<std::mutex> lock(external_mutex);
    // Holding external_mutex
    settings.get("key");  // Acquires internal mutex
    // Lock order: external -> internal
}

// In another thread:
settings.setNotifier([&external_mutex](...) {
    std::lock_guard<std::mutex> lock(external_mutex);  // DEADLOCK!
    // Lock order: internal -> external
});
```

**Solution:** Release external locks before calling GlobalSettings methods.

### 2. Keep Callbacks Fast

```cpp
// Bad: Slow callback blocks all operations
settings.setNotifier([](const std::string& cat,
                        const std::string& key,
                        SettingValue value) {
    // Heavy computation with mutex held
    expensiveOperation();  // Blocks all other threads!
});

// Good: Delegate work to separate thread
std::queue<Event> event_queue;
std::mutex queue_mutex;

settings.setNotifier([&](const std::string& cat,
                         const std::string& key,
                         SettingValue value) {
    // Fast: Just enqueue
    std::lock_guard<std::mutex> lock(queue_mutex);
    event_queue.push({cat, key, value});
});

// Process events asynchronously
std::thread processor([&]() {
    while (running) {
        Event event;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!event_queue.empty()) {
                event = event_queue.front();
                event_queue.pop();
            }
        }
        if (event.valid) {
            expensiveOperation(event);
        }
    }
});
```

### 3. Avoid Long-Running forEach()

```cpp
// Bad: Holds lock for extended time
settings.forEach([](const SettingKey& key, const SettingValue& value) {
    // Long operation blocks all other threads
    slowProcessing(value);
});

// Good: Copy data first, process later
std::vector<std::pair<SettingKey, SettingValue>> snapshot;
settings.forEach([&snapshot](const SettingKey& key, const SettingValue& value) {
    snapshot.push_back({key, value});
});

// Process without holding lock
for (const auto& [key, value] : snapshot) {
    slowProcessing(value);
}
```

### 4. Use Atomic Operations for Flags

```cpp
// Thread-safe flag using GlobalSettings
std::atomic<bool> should_exit{false};

// Writer thread
settings.setNotifier([&](const std::string& cat,
                         const std::string& key,
                         SettingValue value) {
    if (key == "exit_requested") {
        should_exit.store(std::get<bool>(value));
    }
});

// Reader thread (fast, lock-free)
while (!should_exit.load()) {
    // Do work
}
```

## Deadlock Prevention

### Single Lock Design

GlobalSettings uses a single lock, making deadlock impossible within the class itself.

**Guarantee:** Internal operations never deadlock.

### Application Responsibility

Applications must avoid deadlocks when combining GlobalSettings with other locks:

```cpp
// Safe lock ordering
std::mutex app_mutex;

void safeAccess() {
    auto value = settings.get("key");  // Acquire/release GlobalSettings lock

    std::lock_guard<std::mutex> lock(app_mutex);  // Then acquire app lock
    // Process value
}
```

## Testing Thread Safety

### Example Test

```cpp
#include <thread>
#include <vector>

void testConcurrentAccess() {
    GlobalSettings settings;

    const int num_threads = 100;
    const int iterations = 1000;

    std::vector<std::thread> threads;

    // Spawn reader threads
    for (int i = 0; i < num_threads / 2; i++) {
        threads.emplace_back([&settings, iterations]() {
            for (int j = 0; j < iterations; j++) {
                auto value = settings.get("counter");
                // Verify value consistency
            }
        });
    }

    // Spawn writer threads
    for (int i = 0; i < num_threads / 2; i++) {
        threads.emplace_back([&settings, iterations, i]() {
            for (int j = 0; j < iterations; j++) {
                settings.set("counter", SettingValue(i * iterations + j));
            }
        });
    }

    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }

    // No data races, no crashes
    std::cout << "Test passed!" << std::endl;
}
```

## See Also

- [Architecture](architecture.md) - Internal design details
- [API Reference](api-reference.md) - API documentation
- [Examples](examples.md) - Code examples
