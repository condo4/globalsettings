[<img src="https://img.shields.io/badge/yocto-globalsettings-blue.svg?logo=loom">](https://cosgit1.ds.jdsu.net/projects/PLAT_YOCTO/repos/meta-viavi/browse/recipes-core/globalsettings)

# GlobalSettings Class Documentation

## Overview
The `GlobalSettings` class provides a robust system-wide configuration management solution. It allows reading and writing settings from configuration files with category support, type safety, and change notifications.

## Features
- Read settings from multiple configuration files
- Support for different value types (string, int, double, boolean)
- Category-based organization of settings
- File change monitoring with inotify
- Permission-based write access
- Change notification system
- Thread-safe operations
- Python bindings available

## Configuration File Structure
Settings are stored in configuration files with the `.conf` extension. The file format supports:
- Categories in square brackets: `[CategoryName]`
- Key-value pairs: `key = value`
- Owner declarations: `@key = owner_id` (or legacy format: `key/owner = owner_id`)
- Comments: Lines starting with `#` are ignored
- Empty lines are ignored

### Value Types SettingValue
- Strings: `key = "value"` or `key = 'value'` or `key = value`
- Numbers: `key = 42` or `key = 3.14`
- Booleans: `key = true/false` or `key = on/off`

### Example Configuration File

```conf
# Example /etc/system.conf.d/myapp.conf

# General settings (no category)
hostname = server1
debug = false

# Network settings
[network]
port = 8080
host = 0.0.0.0
timeout = 30.0

# Ownership declarations (allow myapp to modify these settings)
@port = myapp
@debug = myapp

# Legacy ownership format also supported
host/owner = myapp
```

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
- `/var/{base}.conf.d/<appid>.conf` - Persistent write data storage

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
 - `/var/system.conf.d/<appid>.conf` - Persistent write data storage (set)

### Custom Base Configuration
You can specify a custom base name to use different configuration files:

```cpp
GlobalSettings settings("myapp");
```

This will use:
 - `/etc/myapp.conf` - System defaults (read-only)
 - `/etc/myapp.conf.d/*` - System configuration fragments (read-only)
 - `/var/myapp.conf.d/<appid>.conf` - Persistent write data storage (set)

This is useful for:
- Multiple independent configuration systems on the same machine
- Application-specific configuration hierarchies
- Testing with isolated configuration sets

## Permissions

Settings must be declared writable using `@key = owner_id` or legacy format `key/owner = owner_id`.
The owner should be the binary filename of the application.
Only the declared owner can modify the setting if the program name matches.

### Settings Command-Line Tool

A CLI tool is provided for viewing and modifying settings (bypasses ownership restrictions for debugging):

```bash
# Display all settings
settings

# Get a specific setting value
settings [category]key

# Set a setting value
settings [category]key value

# Examples:
settings                          # Show all settings
settings hostname                 # Get hostname setting
settings [network]port            # Get port in network category
settings [network]port 8080       # Set port to 8080
settings debug true               # Set debug to true
```

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

# GlobalSettings Python Module Documentation
## Overview

The GlobalSettings module provides a Python interface for managing global settings across your application. It automatically uses the Python script name (from `sys.argv[0]`) as a unique identifier.

## Usage

```python
from GlobalSettings import GlobalSettings
```

## Constructor

### Default Constructor
```python
settings = GlobalSettings()
```

Creates a new GlobalSettings instance using:
- Base name: `"system"` (default)
- Identifier: Current Python script name from `sys.argv[0]`

### Custom Base Constructor
```python
settings = GlobalSettings("fiber_application")
```

Creates a GlobalSettings instance with a custom base name:
- Base name: `"fiber_application"`
- Identifier: Current Python script name from `sys.argv[0]`
- Configuration paths: `/etc/fiber_application.conf`, `/var/fiber_application.conf.d/`, etc.

## Methods

### get(key, category="", default=None)

Retrieves a setting value.

**Parameters:**
- `key` (str): The setting key
- `category` (str, optional): Setting category (default: "")
- `default`: Default value if setting not found (default: None)

**Returns:** The setting value or default if not found

**Supported value types:** str, int, float, bool

```python
# Get with category and default
value = settings.get("my_setting", "my_category", "default_value")

# Get without category
value = settings.get("my_setting", default="default")

# Get with category notation
value = settings.get("[network]port", default=80)
```

### set(key, value, category="")

Sets a setting value. Requires proper ownership declaration in configuration files.

**Parameters:**
- `key` (str): The setting key
- `value`: The value to set (str, int, float, or bool)
- `category` (str, optional): Setting category (default: "")

**Returns:** The set value (note: does not return boolean success indicator)

```python
# Set with category
settings.set("my_setting", "new_value", "my_category")

# Set without category
settings.set("debug", True)

# Set with category notation
settings.set("[network]port", 8080)
```

## Full Example Usage

```python
from GlobalSettings import GlobalSettings

# Example 1: Create settings instance with default "system" base
settings = GlobalSettings()

# Example 2: Create settings instance with custom base
custom_settings = GlobalSettings("fiber_application")
# This uses /etc/fiber_application.conf and /var/fiber_application.conf.d/

# Set values
settings.set("port", 8080, "network")
settings.set("debug", True, "app")

# Get values with defaults
port = settings.get("port", "network", 80)      # returns 8080
debug = settings.get("debug", "app", False)     # returns True
host = settings.get("host", "network", "localhost")  # returns "localhost" if not set

# Using category notation
settings.set("[database]host", "localhost")
db_host = settings.get("[database]host", default="127.0.0.1")
```

## Notes

- The Python script must have proper ownership declarations in configuration files to modify settings
- Settings are persisted to `/var/system.conf.d/<script_name>.conf`
- All settings are read from system configuration files on initialization

## Troubleshooting

### Permission Denied when Setting Values
Ensure your application has an ownership declaration in a configuration file:
```conf
@your_key = your_app_name
```

### Settings Not Persisting
- Check that `/var/system.conf.d/` directory exists and is writable
- Verify your application has proper permissions
- Use the `settings` CLI tool to test: `settings [category]key value`

### File Changes Not Detected
- Ensure inotify is supported on your system
- Check that configuration directories are accessible
- Verify file system supports inotify events

## License

MIT License - See LICENSE file for details
