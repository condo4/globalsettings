# GlobalSettings Class Documentation

## Overview
The `GlobalSettings` class provides a robust system-wide configuration management solution. It allows reading and writing settings from configuration files with category support, type safety, and change notifications.

## Features
- Read settings from multiple configuration files
- Support for different value types (string, int, double, boolean)
- Category-based organization of settings
- File change monitoring
- Permission-based write access
- Change notification system

## Configuration File Structure
Settings are stored in configuration files with the `.conf` extension. The file format supports:
- Categories in square brackets: `[CategoryName]`
- Key-value pairs: `key = value`
- Owner declarations: `@key = owner_id`

### Value Types
- Strings: `key = "value"` or `key = value`
- Numbers: `key = 42` or `key = 3.14`
- Booleans: `key = true/false` or `key = on/off`

## Usage

### Initialization
```cpp
GlobalSettings settings;  // Creates instance and loads configuration
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

### Change Notifications and print helper functionà
```cpp
// Set a notification handler
settings.setNotifier([](const std::string& category, const std::string& key, SettingValue value) {
    // Handle setting change
    std::cout << category << " | " << key << " changed to " << std::visit(setting_value_functor(), value) << std::endl;
});
```

## File Locations
The system reads configuration from:

 - /etc/system.conf - System defaults (read-only)
 - /etc/system.conf.d/* - System configuration fragments (read-only, own by projects packages)
 - /var/system.conf.d/<appid>.conf - Persistant write data storage (set)

## Permissions

Settings must be declared writable using @key = owner_id
The owner should be the binary filename of the application
Only the declared owner can modify the setting if the program name is the good one


The settings command-line tool can bypass ownership restrictions but must used ONLY for debug usage

## Value Types
The system supports these value types through the SettingValue variant:

 - std::string
 - int
 - double
 - bool

# GlobalSettings Python Module Documentation
## Overview

The GlobalSettings module provides a Python interface for managing global settings across your application. It automatically uses the Python script name as a unique identifier.

## Usage Class: GlobalSettings

```python
from GlobalSettings import GlobalSettings
```

## Constructor
```python
settings = GlobalSettings.GlobalSettings()
```

Creates a new GlobalSettings instance using the current Python script name as identifier.

## Methods

### get(key, category="", default=None)

Retrieves a setting value.

    Parameters:
        key (str): The setting key
        category (str, optional): Setting category
        default: Default value if setting not found
    Returns: The setting value or default if not found
    Supported value types: str, int, float, bool

```python
value = settings.get("my_setting", "my_category", "default_value")
```

### set(key, value, category="")

Sets a setting value.

    Parameters:
        key (str): The setting key
        value: The value to set (str, int, float, or bool)
        category (str, optional): Setting category
    Returns: True if successful, False otherwise

```python
settings.set("my_setting", "new_value", "my_category")
```

## Full example usage

```python
from GlobalSettings import GlobalSettings

# Create settings instance
settings = GlobalSettings()

# Set values
settings.set("port", 8080, "network")
settings.set("debug", True, "app")

# Get values
port = settings.get("port", "network", 80)  # returns 8080
debug = settings.get("debug", "app", False)  # returns True
```
