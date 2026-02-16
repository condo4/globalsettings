# GlobalSettings

[![Yocto](https://img.shields.io/badge/yocto-globalsettings-blue.svg?logo=loom)](https://cosgit1.ds.jdsu.net/projects/PLAT_YOCTO/repos/meta-viavi/browse/recipes-core/globalsettings)

## Overview

The `GlobalSettings` library provides a robust, thread-safe system-wide configuration management solution for C++ applications with Python and Qt/QML bindings. It enables reading and writing settings from hierarchical configuration files with category support, type safety, automatic file watching, and change notifications.

## Features

- **Thread-Safe Operations** - All methods are thread-safe for concurrent access
- **Multiple Value Types** - Support for string, int, double, and boolean values
- **Category Organization** - Logical grouping of related settings
- **Automatic File Watching** - Detect and reload configuration changes via inotify
- **Change Notifications** - Get callbacks when settings are modified
- **Ownership Control** - Permission-based write access with ownership declarations
- **Hierarchical Configuration** - Load from multiple sources with precedence
- **Append Operator (+=)** - Concatenate strings or add numbers in configuration files
- **Volatile Variables** - Temporary settings stored in `/tmp/` (not persistent after reboot)
- **Custom Base Names** - Support multiple independent configuration systems
- **Python Bindings** - Full Python API via pybind11
- **Qt/QML Bindings** - Seamless integration with Qt applications and QML property binding
- **CLI Tool** - Command-line utility for viewing and modifying settings

## Documentation

**📚 [Complete Documentation](docs/README.md)** - Start here for comprehensive guides

### Quick Links

| Documentation | Description |
|---------------|-------------|
| **[Getting Started](docs/getting-started.md)** | Installation, quick start, first programs |
| **[API Reference](docs/api-reference.md)** | Complete C++ API documentation |
| **[Configuration Format](docs/configuration.md)** | Configuration file syntax and best practices |
| **[Python Bindings](docs/python-bindings.md)** | Python API and examples |
| **[Qt/QML Bindings](docs/qt_binding.md)** | Qt/QML integration with automatic property binding |
| **[Examples](docs/examples.md)** | Practical code examples for common use cases |
| **[Troubleshooting](docs/troubleshooting.md)** | Solutions to common problems |

### Advanced Topics

| Documentation | Description |
|---------------|-------------|
| **[Architecture](docs/architecture.md)** | Internal design and implementation details |
| **[Thread Safety](docs/threading.md)** | Thread safety guarantees and patterns |
| **[Security](docs/security.md)** | Security considerations and best practices |
| **[Testing](docs/testing.md)** | Testing guide and framework |
| **[Contributing](docs/contributing.md)** | Contribution guidelines and workflow |

## Quick Start

### Installation

```bash
# Clone and build
git clone <repository-url>
cd globalsettings
mkdir build && cd build
cmake .. -DWITH_PYTHON=ON
make
sudo make install
```

### C++ Example

```cpp
#include <globalsettings.h>
#include <iostream>

int main() {
    GlobalSettings settings;

    // Read with defaults
    auto port = std::get<int>(
        settings.get("port", "network", SettingValue(8080))
    );

    std::cout << "Port: " << port << std::endl;

    // Write settings (requires ownership)
    settings.set("debug", SettingValue(true), "app");

    return 0;
}
```

### Python Example

```python
from GlobalSettings import GlobalSettings

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
features = "logging"
features += "database"  # Append to string: "logging database"

counter = 10
counter += 5            # Add to number: 15

# Declare ownership for writable settings (persistent in /var/)
@port = myapp
@debug = myapp

# Declare volatile variables (temporary in /tmp/, lost on reboot)
~temp_value = myapp
temp_value = initial
```

## Configuration File Structure

Settings are stored in configuration files with the `.conf` extension. The file format supports:
- Categories in square brackets: `[CategoryName]`
- Key-value pairs: `key = value`
- Append operator: `key += value` (concatenate strings or add numbers)
- Owner declarations:
  - Regular RW variables: `@key = owner_id` (persistent in `/var/`)
  - Volatile variables: `~key = owner_id` (temporary in `/tmp/`, not persistent after reboot)
  - Legacy format: `key/owner = owner_id` (deprecated)
- Comments: Lines starting with `#` are ignored
- Empty lines are ignored

### Value Types

- Strings: `key = "value"` or `key = 'value'` or `key = value`
- Numbers: `key = 42` or `key = 3.14`
- Booleans: `key = true/false` or `key = on/off`

### Append Operator (+=)

The `+=` operator allows you to append to strings or add to numbers:

**String Concatenation:**
```conf
features = "logging"
features += "database"    # Result: "logging database"
features += "cache"       # Result: "logging database cache"
```

**Numeric Addition:**
```conf
counter = 10
counter += 5              # Result: 15
counter += 3              # Result: 18

price = 19.99
price += 5.00             # Result: 24.99
```

**Key Features:**
- Strings are concatenated with exactly one space (automatic space normalization)
- Numbers are added together (supports int, double, and mixed types)
- Works across configuration file hierarchy
- Multiple appends are supported

**Example across files:**
```conf
# /etc/system.conf
compiler_flags = "-Wall"

# /etc/system.conf.d/dev.conf
compiler_flags += "-g"

# /var/system.conf.d/myapp.conf
compiler_flags += "-DMYAPP"

# Result: compiler_flags = "-Wall -g -DMYAPP"
```

### Configuration Hierarchy

```
/etc/system.conf                    # System defaults (read-only)
/etc/system.conf.d/                 # Configuration fragments (read-only)
  ├── 00-defaults.conf
  ├── 10-network.conf
  └── 20-myapp.conf
/var/system.conf.d/                 # Application settings (read-write, persistent)
  ├── myapp.conf
  └── otherapp.conf
/tmp/system.conf.d/                 # Volatile settings (read-write, temporary)
  ├── myapp.conf
  └── otherapp.conf
```

Settings are loaded in order with later files overriding earlier ones.

**Note:** Settings in `/tmp/` are volatile and will be lost after system reboot. Use volatile variables (`~key = owner`) for temporary runtime configuration that doesn't need to persist.

## Building

### Requirements
- C++20 compatible compiler (GCC 10+, Clang 10+)
- CMake 3.14 or higher
- pybind11 (optional, for Python bindings)
- Python 3.6+ (optional, for Python bindings)

### Build Options
```bash
mkdir build && cd build
cmake .. -DWITH_PYTHON=ON   # Enable Python bindings (default: ON)
cmake .. -DWITH_PYTHON=OFF  # Disable Python bindings
make
sudo make install
```

## Usage

### CMake Integration
```cmake
find_package(GlobalSettings REQUIRED)
include_directories(${LIBGLOBALSETTINGS_INCLUDE_DIR})

...

target_link_libraries(myproject PUBLIC ${LIBGLOBALSETTINGS_LIBRARIES})
```

### Basic Initialization
```cpp
#include <globalsettings.h>

// Default constructor - uses "system" as base name
GlobalSettings settings;  // Loads from /etc/system.conf and /var/system.conf

// Custom base name constructor
GlobalSettings customSettings("fiber_application");
// Loads from /etc/fiber_application.conf and /var/fiber_application.conf
```

The base name determines the configuration file paths:
- `/etc/{base}.conf` - System defaults (read-only)
- `/etc/{base}.conf.d/*` - System configuration fragments (read-only)
- `/var/{base}.conf.d/<appid>.conf` - Persistent write data storage (regular RW variables)
- `/tmp/{base}.conf.d/<appid>.conf` - Volatile write data storage (temporary, lost on reboot)

Default base is `"system"` if using the parameterless constructor.

### Complete C++ Example
```cpp
#include <globalsettings.h>
#include <utilssettings.h>
#include <iostream>

int main() {
    // Example 1: Using default "system" base
    GlobalSettings settings;

    // Example 2: Using custom base name
    GlobalSettings appSettings("myapp");

    // Read settings with defaults
    std::string host = std::get<std::string>(
        settings.get("host", "network", SettingValue("localhost"))
    );
    int port = std::get<int>(
        settings.get("port", "network", SettingValue(8080))
    );

    std::cout << "Server: " << host << ":" << port << std::endl;

    // Modify settings (requires ownership declaration)
    settings.set("port", SettingValue(9000), "network");

    // Set up change notifications
    settings.setNotifier([](const std::string& category,
                           const std::string& key,
                           SettingValue value) {
        std::cout << "Setting changed: [" << category << "]"
                  << key << " = " << valueToStr(value) << std::endl;
    });

    // Iterate over all settings
    std::cout << "\nAll settings:" << std::endl;
    settings.forEach([](const SettingKey& key, const SettingValue& value) {
        std::string category = key.first.empty() ? "" : "[" + key.first + "]";
        std::cout << "  " << category << key.second
                  << " = " << valueToStr(value) << std::endl;
    });

    return 0;
}
```

### Reading Settings
```cpp
// Get value with default
auto value = settings.get("key", "category", defaultValue);

// Get value without category
auto value = settings.get("key");

// Get value using category notation
auto value = settings.get("[category]key");
```

### Writing Settings
```cpp
// Set value with category
settings.set("key", value, "category");

// Set value without category
settings.set("key", value);
```

### Iteration and Size
```cpp
// Get the number of settings
size_t count = settings.size();

// Iterate over all settings safely
settings.forEach([](const SettingKey& key, const SettingValue& value) {
    std::string category = key.first;
    std::string keyName = key.second;
    // Process setting...
});
```

### Instance Identifier
```cpp
// Get the instance ID (by default, the executable filename)
std::string appId = settings.id();

// Set a custom instance ID
settings.setId("my_custom_id");
```

### Utility with SettingValue type: utilssettings.h

**makeValue**: Convert std::string into SettingValue managing numeric, bool and string conversion

```cpp
#include <utilssettings.h>

SettingValue v1 = makeValue("42");      // Integer
SettingValue v2 = makeValue("3.14");    // Double
SettingValue v3 = makeValue("true");    // Boolean
SettingValue v4 = makeValue("text");    // String
```

**valueToStr**: Convert SettingValue into std::string for display
```cpp
#include <utilssettings.h>

std::cout << valueToStr(v1) << std::endl;
```

**valueToInt**: Convert SettingValue into int
```cpp
#include <utilssettings.h>

int intValue = valueToInt(v1);  // Converts string, double, or bool to int
```

### Change Notifications and print helper function
```cpp
#include <globalsettings.h>
#include <utilssettings.h> // For valueToStr()
// Set a notification handler
settings.setNotifier([](const std::string& category, const std::string& key, SettingValue value) {
    // Handle setting change
    std::cout << category << " | " << key << " changed to " << valueToStr(value) << std::endl;
});
```

## File Locations

### Default Configuration (base = "system")
The system reads configuration from:

 - `/etc/system.conf` - System defaults (read-only)
 - `/etc/system.conf.d/*` - System configuration fragments (read-only, owned by project packages)
 - `/var/system.conf.d/<appid>.conf` - Persistent write data storage (regular RW variables)
 - `/tmp/system.conf.d/<appid>.conf` - Volatile write data storage (temporary, lost on reboot)

### Custom Base Configuration
You can specify a custom base name to use different configuration files:

```cpp
GlobalSettings settings("myapp");
```

This will use:
 - `/etc/myapp.conf` - System defaults (read-only)
 - `/etc/myapp.conf.d/*` - System configuration fragments (read-only)
 - `/var/myapp.conf.d/<appid>.conf` - Persistent write data storage (regular RW variables)
 - `/tmp/myapp.conf.d/<appid>.conf` - Volatile write data storage (temporary)

This is useful for:
- Multiple independent configuration systems on the same machine
- Application-specific configuration hierarchies
- Testing with isolated configuration sets

### Variable Types

**Regular RW Variables** (declared with `@key = owner`):
- Stored in `/var/{base}.conf.d/`
- Persistent across reboots
- Suitable for configuration that should be saved permanently
- Example: User preferences, network settings, application state

**Volatile Variables** (declared with `~key = owner`):
- Stored in `/tmp/{base}.conf.d/`
- Temporary storage, cleared on reboot
- Suitable for runtime state that doesn't need persistence
- Example: Session data, temporary flags, runtime counters

## Permissions

Settings must be declared writable using ownership markers:
- **Regular RW variables**: `@key = owner_id` (persistent in `/var/`)
- **Volatile variables**: `~key = owner_id` (temporary in `/tmp/`)
- **Legacy format**: `key/owner = owner_id` (deprecated, use `@key` instead)

The owner should be the binary filename of the application.
Only the declared owner can modify the setting if the program name matches.

### Example Configuration

```conf
[app]
# Persistent setting (survives reboot)
@hostname = myapp
hostname = localhost

# Volatile setting (lost on reboot)
~session_id = myapp
session_id = abc123

# Both can be read and written by myapp
```

### Settings Command-Line Tool

A CLI tool is provided for viewing and modifying settings (bypasses ownership restrictions for debugging):

```bash
# Display all settings
settings
settings -s custom_base          # Use custom base name

# Get a specific setting value
settings [category]key
settings -s myapp [category]key  # Get from custom base

# Set a setting value (works for both regular and volatile variables)
settings [category]key value
settings --source myapp [category]key value

# Show help
settings --help

# Examples:
settings                          # Show all settings (default "system" base)
settings -s myapp                 # Show all settings from "myapp" base
settings hostname                 # Get hostname setting
settings [network]port            # Get port in network category
settings [network]port 8080       # Set port to 8080 (persistent if declared with @)
settings debug true               # Set debug to true (volatile if declared with ~)
settings -s custom host localhost # Set host in custom base
```

**Options:**
- `-s, --source <name>`: Use custom base name (default: "system")
- `-h, --help`: Show help message

**WARNING**: The settings tool bypasses ownership restrictions and should be used ONLY for debugging purposes.

## Value Types
The system supports these value types through the SettingValue variant:

 - std::string
 - int
 - double
 - bool

## Thread Safety

The GlobalSettings class is thread-safe for concurrent operations:
- **get()** operations are safe to call from multiple threads simultaneously
- **set()** operations are internally synchronized
- **forEach()** iteration uses internal locking to ensure thread-safe traversal
- **setNotifier()** callbacks are executed with proper synchronization

Multiple threads can safely read and write settings concurrently. The internal implementation uses mutexes to protect shared data structures.

## Python Bindings

GlobalSettings provides full Python bindings via pybind11. The Python API mirrors the C++ API with automatic type handling.

**Quick Example:**

```python
from GlobalSettings import GlobalSettings

# Create instance
settings = GlobalSettings()

# Read and write settings
port = settings.get("port", "network", 8080)
settings.set("debug", True, "app")

# Custom base name
app_settings = GlobalSettings("myapp")
```

**📖 See [Python Bindings Documentation](docs/python-bindings.md)** for complete API reference, examples, and advanced usage.

## Common Issues

**Need help?** Check the **[Troubleshooting Guide](docs/troubleshooting.md)** for solutions to:
- Installation problems
- Settings not loading or persisting
- Permission errors
- Type mismatches
- Python-specific issues
- Performance problems

**Quick fixes:**

- **Permission Denied:** Ensure ownership is declared in config file: `@your_key = your_app`
- **Settings Not Persisting:** Check `/var/system.conf.d/` exists and is writable
- **File Changes Not Detected:** Verify inotify is supported on your filesystem

## Learn More

### Additional Resources

- **[Complete Documentation](docs/README.md)** - Full documentation index
- **[API Reference](docs/api-reference.md)** - Detailed API documentation
- **[Examples](docs/examples.md)** - Real-world code examples
- **[Architecture](docs/architecture.md)** - Internal design and implementation
- **[Contributing](docs/contributing.md)** - How to contribute to the project

## License

MIT License - See LICENSE file for details
