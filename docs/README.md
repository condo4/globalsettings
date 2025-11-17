# GlobalSettings Documentation

## Overview

Welcome to the GlobalSettings documentation! GlobalSettings is a robust, thread-safe system-wide configuration management library for C++ applications with Python bindings. It provides hierarchical configuration file management, automatic file watching, and change notifications.

## Quick Links

- **[Getting Started](getting-started.md)** - Start here if you're new to GlobalSettings
- **[API Reference](api-reference.md)** - Complete API documentation
- **[Examples](examples.md)** - Practical code examples
- **[Troubleshooting](troubleshooting.md)** - Solutions to common problems

## Documentation Index

### Getting Started

- **[Getting Started Guide](getting-started.md)**
  - Installation instructions
  - Your first C++ program
  - Your first Python program
  - Basic configuration
  - Quick examples

### Core Documentation

- **[API Reference](api-reference.md)**
  - Complete C++ API documentation
  - Class reference (GlobalSettings)
  - Method documentation (get, set, forEach, etc.)
  - Type definitions
  - Utility functions
  - Error handling

- **[Configuration Format](configuration.md)**
  - File structure and syntax
  - Value types (string, int, double, bool)
  - Categories and organization
  - Ownership declarations
  - File locations and hierarchy
  - Best practices
  - Complete examples

- **[Python Bindings](python-bindings.md)**
  - Python API reference
  - Installation and setup
  - Basic usage
  - Complete examples
  - Type handling
  - Configuration management scripts
  - Limitations and workarounds

### Advanced Topics

- **[Architecture and Design](architecture.md)**
  - Internal architecture
  - Core components
  - Configuration loading
  - Thread safety design
  - File watching implementation
  - Ownership system
  - Type system
  - Performance considerations
  - Extension points

- **[Thread Safety](threading.md)**
  - Thread safety guarantees
  - Internal synchronization
  - Common concurrent patterns
  - Performance considerations
  - Best practices
  - Deadlock prevention
  - Testing thread safety

- **[Security Considerations](security.md)**
  - Ownership system
  - File system security
  - Sensitive data handling
  - Path traversal prevention
  - Injection vulnerability prevention
  - Input validation
  - Audit logging
  - Security checklist

### Practical Guides

- **[Code Examples](examples.md)**
  - Basic usage examples
  - Advanced patterns
  - Real-world applications
  - Python examples
  - Error handling
  - Configuration wrappers
  - Web server configurations
  - Database connection pools

- **[Troubleshooting Guide](troubleshooting.md)**
  - Installation problems
  - Runtime issues
  - Configuration file issues
  - Performance problems
  - Python-specific issues
  - Debugging tools
  - Getting help

### Development

- **[Testing Documentation](testing.md)**
  - Test structure
  - Running tests
  - Test environment setup
  - Test cases overview
  - Writing new tests
  - Continuous integration
  - Performance testing
  - Debugging tests

- **[Contributing Guide](contributing.md)**
  - Development setup
  - Code style guidelines
  - Making changes
  - Adding features
  - Writing tests
  - Documentation
  - Code review process
  - Reporting issues

## Feature Highlights

### Core Features

- **Thread-Safe Operations** - All operations are thread-safe with internal mutex protection
- **Multiple Value Types** - Support for string, int, double, and boolean values
- **Category Organization** - Organize settings into logical categories
- **Automatic File Watching** - Detect and reload configuration changes automatically
- **Change Notifications** - Get notified when settings change
- **Ownership Control** - Prevent unauthorized modifications with ownership declarations
- **Hierarchical Configuration** - Load from multiple sources with precedence
- **Custom Base Names** - Support multiple independent configuration systems

### Platform Support

- **Linux** - Primary platform with full inotify support
- **C++20** - Modern C++ with std::variant and other features
- **Python 3.6+** - Full Python bindings via pybind11

## Quick Start

### C++ Example

```cpp
#include <globalsettings.h>
#include <iostream>

int main() {
    // Create settings instance
    GlobalSettings settings;

    // Read with defaults
    auto port = std::get<int>(
        settings.get("port", "network", SettingValue(8080))
    );

    std::cout << "Port: " << port << std::endl;

    // Write settings
    settings.set("debug", SettingValue(true), "app");

    return 0;
}
```

### Python Example

```python
from GlobalSettings import GlobalSettings

# Create settings instance
settings = GlobalSettings()

# Read with defaults
port = settings.get("port", "network", 8080)
print(f"Port: {port}")

# Write settings
settings.set("debug", True, "app")
```

### Configuration File

```conf
# /etc/system.conf.d/myapp.conf

[network]
port = 8080
host = 0.0.0.0

[app]
debug = false
log_level = info

# Declare ownership
@port = myapp
@debug = myapp
```

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│              Application Layer                       │
│  ┌──────────────────────────────────────────────┐  │
│  │         GlobalSettings API                    │  │
│  │  get() | set() | forEach() | setNotifier()   │  │
│  └──────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│              Core Services                           │
│  ┌──────────────┐  ┌────────────────────────────┐ │
│  │ Thread Safety│  │   Ownership Control        │ │
│  │  (mutex)     │  │   (validation)             │ │
│  └──────────────┘  └────────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│              Storage Layer                           │
│  ┌──────────────┐  ┌────────────────────────────┐ │
│  │ Settings Map │  │   Owners Map               │ │
│  └──────────────┘  └────────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│              File System Layer                       │
│  ┌──────────────┐  ┌────────────────────────────┐ │
│  │ChangeWatcher │  │   File I/O                 │ │
│  │  (inotify)   │  │   (read/write)             │ │
│  └──────────────┘  └────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

## Configuration Hierarchy

```
/etc/system.conf                    # System defaults (read-only)
/etc/system.conf.d/                 # Configuration fragments (read-only)
  ├── 00-defaults.conf
  ├── 10-network.conf
  └── 20-app.conf
/var/system.conf.d/                 # Application data (read-write)
  ├── myapp.conf
  └── otherapp.conf
```

Settings are loaded in order with later files overriding earlier ones.

## Common Use Cases

### Web Application Configuration

```cpp
GlobalSettings settings("webapp");

// Network configuration
auto host = std::get<std::string>(settings.get("host", "server", SettingValue("0.0.0.0")));
auto port = std::get<int>(settings.get("port", "server", SettingValue(8080)));

// Database configuration
auto db_host = std::get<std::string>(settings.get("host", "database", SettingValue("localhost")));
auto db_port = std::get<int>(settings.get("port", "database", SettingValue(5432)));

// Feature flags
auto enable_cache = std::get<bool>(settings.get("enable_cache", "features", SettingValue(true)));
```

### Runtime Configuration Updates

```cpp
// Monitor configuration changes
settings.setNotifier([](const std::string& category,
                        const std::string& key,
                        SettingValue value) {
    if (key == "debug" && category == "app") {
        bool debug = std::get<bool>(value);
        if (debug) {
            enableDebugLogging();
        } else {
            disableDebugLogging();
        }
    }
});

// Changes detected automatically via inotify
```

### Multi-Application Configuration

```cpp
// Different applications use different base names
GlobalSettings web_settings("webapp");
GlobalSettings api_settings("api");
GlobalSettings worker_settings("worker");

// Each has independent configuration
// /etc/webapp.conf, /etc/api.conf, /etc/worker.conf
```

## Best Practices

1. **Use Custom Base Names** - For application-specific configuration
2. **Declare Ownership** - For all writable settings
3. **Validate Input** - Always validate configuration values
4. **Provide Defaults** - Use meaningful defaults in get() calls
5. **Cache Hot Paths** - Cache frequently accessed settings
6. **Handle Errors** - Check return values and handle failures gracefully
7. **Document Settings** - Comment configuration files
8. **Security First** - Never store secrets in config files
9. **Test Thread Safety** - Test concurrent access patterns
10. **Monitor Changes** - Use notifiers for dynamic reconfiguration

## Performance Tips

- **Cache frequently accessed settings** to reduce lock contention
- **Batch updates** instead of many individual set() calls
- **Use local copies** for read-heavy workloads
- **Keep callbacks fast** - Don't block in notifier callbacks
- **Avoid long forEach()** - Copy data first if processing takes time

## Limitations

### Current Limitations

- **Linux only** - inotify is Linux-specific
- **Single machine** - No distributed configuration
- **File-based** - No database backend
- **Python limited API** - No forEach() or setNotifier() in Python
- **Text format only** - No binary configuration support

### Workarounds

- For non-Linux: Disable file watching or use polling
- For distributed: Use external configuration management
- For databases: Wrap GlobalSettings with custom backend
- For Python: Use CLI tool for iteration
- For binary: Store references to binary files

## Additional Resources

### External Links

- Project Repository: [GitHub URL]
- Issue Tracker: [Issues URL]
- Discussions: [Discussions URL]
- License: MIT License

### Community

- Ask questions in GitHub Discussions
- Report bugs in Issue Tracker
- Contribute via Pull Requests
- Follow contribution guidelines

## Support

### Getting Help

1. Check the [Troubleshooting Guide](troubleshooting.md)
2. Search existing [GitHub Issues]
3. Review [Examples](examples.md)
4. Ask in [GitHub Discussions]
5. Contact maintainers

### Reporting Issues

See [Contributing Guide](contributing.md) for:
- Bug report template
- Feature request process
- Security issue reporting

## Version Information

- **Current Version:** 1.1.0
- **C++ Standard:** C++20
- **Python Support:** Python 3.6+
- **Platform:** Linux

## License

GlobalSettings is released under the MIT License. See LICENSE file for details.

---

## Documentation Navigation

| Section | Description |
|---------|-------------|
| [Getting Started](getting-started.md) | Quick start guide for new users |
| [API Reference](api-reference.md) | Complete API documentation |
| [Configuration](configuration.md) | Configuration file format and syntax |
| [Python Bindings](python-bindings.md) | Python API documentation |
| [Architecture](architecture.md) | Internal design and architecture |
| [Thread Safety](threading.md) | Thread safety guarantees and patterns |
| [Security](security.md) | Security considerations and best practices |
| [Examples](examples.md) | Practical code examples |
| [Troubleshooting](troubleshooting.md) | Common issues and solutions |
| [Testing](testing.md) | Testing guide and framework |
| [Contributing](contributing.md) | Contribution guidelines |

---

**Last Updated:** 2025-11-17
**Documentation Version:** 1.1.0
