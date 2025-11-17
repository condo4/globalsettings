# Getting Started with GlobalSettings

## Quick Overview

GlobalSettings is a thread-safe system-wide configuration management library for C++ applications. It provides:
- Hierarchical configuration file management
- Category-based setting organization
- Type-safe value handling (string, int, double, bool)
- File change monitoring with automatic reload
- Thread-safe operations
- Python bindings

## Installation

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+)
- CMake 3.14 or higher
- pybind11 (optional, for Python bindings)
- Python 3.6+ (optional, for Python bindings)

### Build Steps

```bash
# Clone the repository
git clone <repository-url>
cd globalsettings

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DWITH_PYTHON=ON   # Enable Python bindings (default: ON)
# or
cmake .. -DWITH_PYTHON=OFF  # Disable Python bindings

# Build
make

# Install (requires sudo)
sudo make install
```

## Your First C++ Program

### Basic Example

```cpp
#include <globalsettings.h>
#include <utilssettings.h>
#include <iostream>

int main() {
    // Create a GlobalSettings instance
    // Uses default "system" base name
    GlobalSettings settings;

    // Read a setting with a default value
    auto hostValue = settings.get("host", "network", SettingValue("localhost"));
    std::string host = std::get<std::string>(hostValue);

    auto portValue = settings.get("port", "network", SettingValue(8080));
    int port = std::get<int>(portValue);

    std::cout << "Server: " << host << ":" << port << std::endl;

    return 0;
}
```

### Compile and Link

```bash
# Using g++
g++ -std=c++20 myapp.cpp -lglobalsettings -o myapp

# Using CMake
find_package(GlobalSettings REQUIRED)
target_link_libraries(myapp PUBLIC ${LIBGLOBALSETTINGS_LIBRARIES})
```

## Your First Python Program

```python
from GlobalSettings import GlobalSettings

# Create settings instance
settings = GlobalSettings()

# Read settings with defaults
host = settings.get("host", "network", "localhost")
port = settings.get("port", "network", 8080)

print(f"Server: {host}:{port}")
```

## Configuration Files

### File Locations

GlobalSettings reads from multiple locations in order:

1. `/etc/system.conf` - System-wide defaults
2. `/etc/system.conf.d/*.conf` - Configuration fragments
3. `/var/system.conf.d/<appid>.conf` - Application-specific settings

### Basic Configuration File

Create `/etc/system.conf.d/myapp.conf`:

```conf
# Network settings
[network]
host = 0.0.0.0
port = 8080
timeout = 30.0

# Application settings
[app]
debug = false
log_level = info

# Allow myapp to modify these settings
@port = myapp
@debug = myapp
@log_level = myapp
```

## Custom Base Name

Instead of using the default "system" base, you can specify a custom base name:

```cpp
// C++
GlobalSettings appSettings("myapp");
// Uses /etc/myapp.conf and /var/myapp.conf.d/
```

```python
# Python
settings = GlobalSettings("myapp")
# Uses /etc/myapp.conf and /var/myapp.conf.d/
```

This is useful for:
- Isolating configuration for different applications
- Testing with separate configuration sets
- Managing multiple independent configuration systems

## Next Steps

- [Configuration Format](configuration.md) - Learn the detailed configuration file syntax
- [API Reference](api-reference.md) - Complete API documentation
- [Examples](examples.md) - More code examples
- [Python Bindings](python-bindings.md) - Python-specific documentation
- [Thread Safety](threading.md) - Understanding concurrent access
