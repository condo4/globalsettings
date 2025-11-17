# Architecture and Design

## Overview

GlobalSettings is designed as a thread-safe, hierarchical configuration management system with file watching capabilities. This document describes the internal architecture and design decisions.

## Core Components

### 1. GlobalSettings Class

The main public interface providing configuration management.

**Key responsibilities:**
- Setting read/write operations
- Thread synchronization
- Ownership validation
- Change notification dispatch

**Key design patterns:**
- **Facade pattern**: Simplifies access to complex subsystems
- **Observer pattern**: Change notifications to registered callbacks
- **Thread-safe singleton-like behavior**: Each instance manages its own state safely

### 2. ChangeWatcher Class

Monitors configuration files for changes using inotify.

**Key responsibilities:**
- File system monitoring
- Change detection
- Automatic configuration reload
- Directory watching

**Implementation details:**
- Uses Linux inotify API
- Watches both `/etc` and `/var` directories
- Separate thread for file monitoring
- Debounces rapid file changes

### 3. Settings Storage

Internal data structures for configuration storage.

```cpp
// Simplified representation
class GlobalSettings {
private:
    // Category -> Key -> Value mapping
    std::map<std::string, std::map<std::string, SettingValue>> settings_;

    // Ownership tracking
    std::map<SettingKey, std::string> owners_;

    // Thread synchronization
    mutable std::mutex mutex_;

    // Change notification
    std::function<void(...)> notifier_;
};
```

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
├─────────────────────────────────────────────────────────┤
│                  GlobalSettings API                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │   get()  │ │  set()   │ │ forEach()│ │setNotifier│ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
├─────────────────────────────────────────────────────────┤
│                   Core Services                          │
│  ┌─────────────────────┐  ┌──────────────────────┐    │
│  │  Thread Safety      │  │  Ownership Control   │    │
│  │  (std::mutex)       │  │  (Validation)        │    │
│  └─────────────────────┘  └──────────────────────┘    │
├─────────────────────────────────────────────────────────┤
│                   Storage Layer                          │
│  ┌─────────────────────┐  ┌──────────────────────┐    │
│  │  Settings Map       │  │  Owners Map          │    │
│  │  (std::map)         │  │  (std::map)          │    │
│  └─────────────────────┘  └──────────────────────┘    │
├─────────────────────────────────────────────────────────┤
│                   File System Layer                      │
│  ┌─────────────────────┐  ┌──────────────────────┐    │
│  │  ChangeWatcher      │  │  File I/O            │    │
│  │  (inotify)          │  │  (read/write)        │    │
│  └─────────────────────┘  └──────────────────────┘    │
└─────────────────────────────────────────────────────────┘
           ↓                         ↓
    /etc/{base}.conf           /var/{base}.conf.d/
```

## Configuration Loading

### Initialization Sequence

1. **Constructor called**
   ```cpp
   GlobalSettings settings("system");
   ```

2. **Load system defaults**
   - Read `/etc/system.conf`
   - Parse settings and ownership

3. **Load configuration fragments**
   - Read all files in `/etc/system.conf.d/`
   - Process in alphabetical order
   - Override duplicate keys

4. **Load application settings**
   - Read `/var/system.conf.d/{appid}.conf`
   - Override with application-specific values

5. **Start file watcher**
   - Initialize inotify
   - Watch all configuration directories
   - Start monitoring thread

### File Loading Algorithm

```
function loadConfiguration():
    settings = {}
    owners = {}

    // Load in order of precedence
    for each file in [
        "/etc/{base}.conf",
        "/etc/{base}.conf.d/*.conf" (sorted),
        "/var/{base}.conf.d/{appid}.conf"
    ]:
        if file exists:
            (fileSettings, fileOwners) = parseFile(file)
            settings.merge(fileSettings)  // Later values override
            owners.merge(fileOwners)

    return (settings, owners)
```

## Thread Safety Design

### Synchronization Strategy

GlobalSettings uses **coarse-grained locking** with a single mutex protecting all shared state.

**Rationale:**
- Configuration access is typically infrequent
- Simplicity over maximum concurrency
- Avoids deadlock complexity
- Adequate performance for typical use cases

### Lock Granularity

```cpp
class GlobalSettings {
private:
    mutable std::mutex mutex_;  // Guards all member data

public:
    SettingValue get(...) const {
        std::lock_guard<std::mutex> lock(mutex_);
        // Access settings_
    }

    bool set(...) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Modify settings_
    }

    void forEach(...) const {
        std::lock_guard<std::mutex> lock(mutex_);
        // Iterate settings_
    }
};
```

### Thread Safety Guarantees

- **Data race free**: All shared state protected by mutex
- **Deadlock free**: Single lock, no lock ordering issues
- **Progress guarantee**: Lock-free waiting not needed for this use case

## File Watching Implementation

### inotify Integration

ChangeWatcher uses Linux inotify for efficient file monitoring:

```cpp
class ChangeWatcher {
private:
    int inotify_fd_;              // inotify file descriptor
    std::thread watch_thread_;     // Monitoring thread
    std::map<int, std::string> watch_descriptors_;  // wd -> path

public:
    void start() {
        // Create inotify instance
        inotify_fd_ = inotify_init1(IN_NONBLOCK);

        // Add watches for directories
        addWatch("/etc/{base}.conf.d");
        addWatch("/var/{base}.conf.d");

        // Start monitoring thread
        watch_thread_ = std::thread(&ChangeWatcher::monitorLoop, this);
    }

    void monitorLoop() {
        while (running_) {
            // Read inotify events
            // Parse events
            // Trigger reload on file changes
        }
    }
};
```

### Event Handling

```
File Change Event
    ↓
inotify notification
    ↓
ChangeWatcher detects
    ↓
Reload configuration
    ↓
Notify callbacks
```

## Ownership System

### Design Goals

1. **Security**: Prevent unauthorized modifications
2. **Flexibility**: Different owners for different settings
3. **Simplicity**: Easy to declare and verify

### Ownership Validation

```cpp
bool GlobalSettings::set(const std::string& key,
                         const SettingValue& value,
                         const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);

    SettingKey sk{category, key};

    // Check if ownership required
    if (owners_.count(sk) > 0) {
        std::string required_owner = owners_[sk];

        // Verify current application is owner
        if (id() != required_owner) {
            return false;  // Not authorized
        }
    }

    // Update setting
    settings_[category][key] = value;

    // Persist to file
    writeToFile(category, key, value);

    // Notify listeners
    if (notifier_) {
        notifier_(category, key, value);
    }

    return true;
}
```

## Type System

### SettingValue Variant

```cpp
using SettingValue = std::variant<std::string, int, double, bool>;
```

**Design rationale:**
- Type safety without runtime overhead
- Efficient storage (union-like)
- Visitor pattern support
- Modern C++ idiom (C++17+)

### Type Inference

```cpp
SettingValue makeValue(const std::string& str) {
    // Try bool
    if (str == "true" || str == "false" || ...) {
        return SettingValue{str == "true"};
    }

    // Try int
    if (isInteger(str)) {
        return SettingValue{std::stoi(str)};
    }

    // Try double
    if (isFloat(str)) {
        return SettingValue{std::stod(str)};
    }

    // Default to string
    return SettingValue{str};
}
```

## Error Handling Strategy

### Design Principles

1. **Non-throwing by default**: Return error codes/defaults
2. **Graceful degradation**: Continue with defaults on errors
3. **Logging for diagnostics**: Record errors for debugging
4. **Recovery when possible**: Retry transient failures

### Error Scenarios

| Scenario | Handling |
|----------|----------|
| File not found | Use defaults, log warning |
| Parse error | Skip line, continue parsing |
| Permission denied (read) | Use cached values |
| Permission denied (write) | Return false, log error |
| inotify failure | Disable watching, continue operation |

## Performance Considerations

### Optimization Strategies

1. **Lazy loading**: Load only when needed
2. **Caching**: Keep parsed configuration in memory
3. **Efficient data structures**: std::map for O(log n) lookups
4. **Minimal file I/O**: Write only on set()
5. **Coarse locking**: Simple, adequate performance

### Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| get() | O(log n) | Map lookup |
| set() | O(log n) + I/O | Map insert + file write |
| forEach() | O(n) | Iterate all settings |
| File reload | O(m) | Parse m lines |

### Scalability

The design scales well for typical configuration use cases:
- **Settings count**: Up to 10,000s of settings
- **File size**: Up to megabytes
- **Concurrent access**: Dozens of threads
- **Update frequency**: Occasional (not high-frequency)

## Extension Points

### Future Enhancements

Potential areas for extension:

1. **Alternative backends**: Database, remote configuration service
2. **Encryption**: Encrypted setting values
3. **Validation**: Schema validation for settings
4. **Transactions**: Atomic multi-setting updates
5. **Namespace isolation**: Per-namespace configuration
6. **Hot reload hooks**: Application callbacks on reload

## Design Trade-offs

### Decisions Made

| Choice | Alternative | Rationale |
|--------|-------------|-----------|
| Coarse locking | Fine-grained locks | Simplicity, adequate performance |
| inotify | Polling | Efficiency, real-time updates |
| std::map | std::unordered_map | Deterministic iteration order |
| Single file per app | Shared file | Isolation, concurrent writes |
| Variant types | void* or inheritance | Type safety, modern C++ |

## See Also

- [API Reference](api-reference.md) - Public API documentation
- [Thread Safety](threading.md) - Thread safety details
- [Security](security.md) - Security considerations
