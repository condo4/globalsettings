# Python Bindings

## Overview

GlobalSettings provides Python bindings via pybind11, offering a Pythonic interface to the C++ library. The Python module automatically uses the script name from `sys.argv[0]` as the application identifier.

## Installation

### Building with Python Support

```bash
mkdir build && cd build
cmake .. -DWITH_PYTHON=ON
make
sudo make install
```

### Verify Installation

```python
import GlobalSettings
print(GlobalSettings.__version__)
```

## Basic Usage

### Creating a GlobalSettings Instance

```python
from GlobalSettings import GlobalSettings

# Default constructor (uses "system" base)
settings = GlobalSettings()

# Custom base constructor
app_settings = GlobalSettings("myapp")
```

### Instance Identifier

The Python bindings automatically set the instance ID to the script name from `sys.argv[0]`:

```python
# If running: python3 /usr/local/bin/myapp.py
# Instance ID will be: myapp.py
# Settings will be saved to: /var/system.conf.d/myapp.py.conf
```

## API Reference

### get(key, category="", default=None)

Retrieves a setting value.

**Parameters:**
- `key` (str): Setting key name
- `category` (str, optional): Category name (default: "")
- `default` (any, optional): Default value if not found (default: None)

**Returns:** Setting value or default

**Supported types:** str, int, float, bool

```python
# Get with category and default
port = settings.get("port", "network", 8080)

# Get without category
debug = settings.get("debug", default=False)

# Get with category notation
host = settings.get("[network]host", default="localhost")

# Type inference from default value
timeout = settings.get("timeout", "network", 30.0)  # Returns float
```

### set(key, value, category="")

Sets a setting value.

**Parameters:**
- `key` (str): Setting key name
- `value` (str, int, float, bool): Value to set
- `category` (str, optional): Category name (default: "")

**Returns:** The value that was set

**Requires:** Ownership declaration in configuration files

```python
# Set with category
settings.set("port", 9000, "network")

# Set without category
settings.set("debug", True)

# Set with category notation
settings.set("[network]timeout", 60.0)

# Set different types
settings.set("name", "MyApp")           # String
settings.set("count", 42)                # Integer
settings.set("ratio", 3.14)              # Float
settings.set("enabled", True)            # Boolean
```

## Complete Examples

### Basic Application

```python
#!/usr/bin/env python3
from GlobalSettings import GlobalSettings

def main():
    # Create settings instance
    settings = GlobalSettings()

    # Read configuration
    host = settings.get("host", "network", "0.0.0.0")
    port = settings.get("port", "network", 8080)
    debug = settings.get("debug", "app", False)

    print(f"Starting server on {host}:{port}")
    print(f"Debug mode: {debug}")

    # Update settings
    if debug:
        settings.set("log_level", "debug", "app")
    else:
        settings.set("log_level", "info", "app")

if __name__ == "__main__":
    main()
```

### Web Application Configuration

```python
#!/usr/bin/env python3
from GlobalSettings import GlobalSettings
import sys

class AppConfig:
    def __init__(self, base="system"):
        self.settings = GlobalSettings(base)

    @property
    def database_url(self):
        host = self.settings.get("host", "database", "localhost")
        port = self.settings.get("port", "database", 5432)
        db = self.settings.get("database", "database", "myapp")
        return f"postgresql://{host}:{port}/{db}"

    @property
    def debug_mode(self):
        return self.settings.get("debug", "app", False)

    def enable_feature(self, feature_name, enabled=True):
        """Enable or disable a feature flag"""
        self.settings.set(feature_name, enabled, "features")

    def is_feature_enabled(self, feature_name):
        """Check if a feature is enabled"""
        return self.settings.get(feature_name, "features", False)

# Usage
config = AppConfig("myapp")
print(f"Database: {config.database_url}")
print(f"Debug: {config.debug_mode}")

# Feature flags
config.enable_feature("beta_ui", True)
if config.is_feature_enabled("beta_ui"):
    print("Beta UI enabled")
```

### Configuration Management Script

```python
#!/usr/bin/env python3
"""
Settings management utility
"""
from GlobalSettings import GlobalSettings
import argparse
import sys

def show_all_settings(settings):
    """Display all settings (note: forEach not available in Python)"""
    print("Settings stored in: /var/system.conf.d/")
    print("\nCurrent settings can be viewed with: settings command")

def get_setting(settings, key, category=""):
    """Get and display a setting"""
    value = settings.get(key, category)
    if value is None:
        print(f"Setting not found: [{category}]{key}")
        return False

    print(f"[{category}]{key} = {value}")
    return True

def set_setting(settings, key, value, category=""):
    """Set a setting"""
    try:
        # Try to infer type
        if value.lower() in ('true', 'false', 'on', 'off', 'yes', 'no'):
            value = value.lower() in ('true', 'on', 'yes')
        elif '.' in value:
            try:
                value = float(value)
            except ValueError:
                pass  # Keep as string
        else:
            try:
                value = int(value)
            except ValueError:
                pass  # Keep as string

        settings.set(key, value, category)
        print(f"Set [{category}]{key} = {value}")
        return True
    except Exception as e:
        print(f"Error setting value: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="GlobalSettings management utility")
    parser.add_argument("--base", default="system", help="Configuration base name")
    parser.add_argument("--category", "-c", default="", help="Setting category")
    parser.add_argument("key", nargs="?", help="Setting key")
    parser.add_argument("value", nargs="?", help="Setting value")

    args = parser.parse_args()

    settings = GlobalSettings(args.base)

    if args.key is None:
        # Show all settings
        show_all_settings(settings)
    elif args.value is None:
        # Get setting
        if not get_setting(settings, args.key, args.category):
            sys.exit(1)
    else:
        # Set setting
        if not set_setting(settings, args.key, args.value, args.category):
            sys.exit(1)

if __name__ == "__main__":
    main()
```

Usage:
```bash
# Get a setting
./manage_settings.py -c network port

# Set a setting
./manage_settings.py -c network port 9000

# Use custom base
./manage_settings.py --base myapp -c app debug true
```

## Type Handling

### Automatic Type Conversion

Python bindings handle type conversion automatically:

```python
# Reading
value = settings.get("count", default=0)  # Returns int
value = settings.get("ratio", default=0.0)  # Returns float
value = settings.get("name", default="")  # Returns str
value = settings.get("enabled", default=False)  # Returns bool

# Writing - type preserved
settings.set("count", 42)          # Stored as int
settings.set("ratio", 3.14)        # Stored as float
settings.set("name", "test")       # Stored as string
settings.set("enabled", True)      # Stored as bool
```

### Type Safety

```python
# Get with specific type default
port = settings.get("port", "network", 8080)

# Verify type
if isinstance(port, int):
    print(f"Port: {port}")
else:
    print("Port is not an integer!")
```

## Configuration Files

Python applications use the same configuration format as C++ applications:

```conf
# /etc/system.conf.d/python-app.conf

[app]
debug = false
workers = 4

[network]
host = 0.0.0.0
port = 8080

# Declare ownership for Python script
@debug = myscript.py
@workers = myscript.py
@port = myscript.py
```

## Best Practices

### 1. Use Type Hints

```python
from typing import Optional

def get_port(settings: GlobalSettings, default: int = 8080) -> int:
    """Get port setting with type checking"""
    port = settings.get("port", "network", default)
    if not isinstance(port, int):
        return default
    return port
```

### 2. Create Configuration Classes

```python
from dataclasses import dataclass

@dataclass
class NetworkConfig:
    host: str = "0.0.0.0"
    port: int = 8080
    timeout: float = 30.0

    @classmethod
    def from_settings(cls, settings: GlobalSettings):
        return cls(
            host=settings.get("host", "network", cls.host),
            port=settings.get("port", "network", cls.port),
            timeout=settings.get("timeout", "network", cls.timeout)
        )
```

### 3. Error Handling

```python
def safe_set(settings, key, value, category=""):
    """Set with error handling"""
    try:
        settings.set(key, value, category)
        return True
    except Exception as e:
        print(f"Failed to set {key}: {e}")
        return False
```

### 4. Use Context Managers

```python
class SettingsContext:
    def __init__(self, base="system"):
        self.settings = GlobalSettings(base)
        self.original_values = {}

    def __enter__(self):
        return self.settings

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Restore original values if needed
        pass

# Usage
with SettingsContext("myapp") as settings:
    settings.set("debug", True)
    # ... do work ...
```

## Troubleshooting

### Import Error

```python
# Error: No module named 'GlobalSettings'
import sys
print(sys.path)  # Check Python path

# Add install location if needed
sys.path.insert(0, '/usr/local/lib/python3.x/site-packages')
```

### Permission Denied

```python
# Ensure ownership is declared in config file
# /etc/system.conf.d/myapp.conf
# @setting_name = myscript.py

# Check script name matches
import sys
print(f"Script name: {sys.argv[0]}")
```

### Type Mismatches

```python
# Always provide typed defaults
value = settings.get("port", "network", 8080)  # Default ensures int type

# Or check type explicitly
value = settings.get("port", "network")
if isinstance(value, int):
    port = value
else:
    port = 8080
```

## Limitations

Current Python binding limitations:

1. **No forEach()**: Cannot iterate over all settings in Python
2. **No setNotifier()**: Change notifications not available in Python
3. **No size()**: Cannot get count of all settings

For these features, use the C++ API directly or use the `settings` CLI tool.

## See Also

- [API Reference](api-reference.md) - C++ API documentation
- [Configuration Format](configuration.md) - Configuration file syntax
- [Examples](examples.md) - More code examples
