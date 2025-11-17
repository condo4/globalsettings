# Troubleshooting Guide

## Common Issues

### Installation Problems

#### Build Fails with C++ Standard Error

**Symptom:**
```
error: 'variant' in namespace 'std' does not name a type
```

**Cause:** Compiler doesn't support C++20 or C++20 not enabled.

**Solution:**
```bash
# Ensure C++20 is enabled
cmake .. -DCMAKE_CXX_STANDARD=20

# Or use a newer compiler
export CXX=g++-10  # GCC 10+
export CXX=clang++-10  # Clang 10+
cmake .. && make
```

#### Python Bindings Not Building

**Symptom:**
```
Could NOT find pybind11
```

**Cause:** pybind11 not installed.

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install pybind11-dev python3-pybind11

# Fedora/RHEL
sudo dnf install pybind11-devel

# Or via pip
pip3 install pybind11

# Rebuild
cmake .. -DWITH_PYTHON=ON
make
```

#### Installation Permission Denied

**Symptom:**
```
CMake Error at cmake_install.cmake:XX (file):
  file INSTALL cannot copy file
```

**Cause:** Insufficient permissions to install to system directories.

**Solution:**
```bash
# Use sudo for system-wide installation
sudo make install

# Or install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install
```

### Runtime Problems

#### Settings Not Loading

**Symptom:**
- `get()` always returns default values
- Configuration appears empty

**Diagnosis:**
```bash
# Check if configuration files exist
ls -la /etc/system.conf
ls -la /etc/system.conf.d/
ls -la /var/system.conf.d/

# View effective configuration
settings

# Check file permissions
ls -l /etc/system.conf
```

**Common causes:**

1. **Files don't exist**
   ```bash
   # Create configuration directory
   sudo mkdir -p /etc/system.conf.d
   sudo mkdir -p /var/system.conf.d
   sudo chmod 755 /var/system.conf.d
   ```

2. **Wrong base name**
   ```cpp
   // Wrong: Using "myapp" but files are named "system.conf"
   GlobalSettings settings("myapp");

   // Fix: Use correct base name
   GlobalSettings settings("system");
   // Or rename files to /etc/myapp.conf
   ```

3. **Permission issues**
   ```bash
   # Ensure files are readable
   sudo chmod 644 /etc/system.conf
   sudo chmod 755 /etc/system.conf.d
   ```

#### Cannot Write Settings

**Symptom:**
- `set()` returns false
- Settings don't persist

**Diagnosis:**
```cpp
// Check return value
bool success = settings.set("key", SettingValue(42), "category");
if (!success) {
    std::cerr << "Failed to write setting" << std::endl;
}

// Check instance ID
std::cout << "Instance ID: " << settings.id() << std::endl;
```

**Common causes:**

1. **Missing ownership declaration**

   Configuration file missing `@key = owner`:
   ```conf
   # Bad: No ownership
   [network]
   port = 8080
   ```

   ```conf
   # Good: Ownership declared
   [network]
   port = 8080
   @port = myapp
   ```

2. **Wrong instance ID**

   Check your application name matches ownership:
   ```cpp
   GlobalSettings settings;
   std::cout << "ID: " << settings.id() << std::endl;
   // Must match @key = <this_id>
   ```

3. **Directory not writable**
   ```bash
   # Check permissions
   ls -ld /var/system.conf.d
   # Should be: drwxr-xr-x or drwxrwxr-x

   # Fix permissions
   sudo chmod 755 /var/system.conf.d
   ```

4. **File permissions**
   ```bash
   # If application file exists, check permissions
   ls -l /var/system.conf.d/myapp.conf

   # Fix if needed
   sudo chown myapp:myapp /var/system.conf.d/myapp.conf
   sudo chmod 644 /var/system.conf.d/myapp.conf
   ```

#### File Changes Not Detected

**Symptom:**
- Modify configuration file manually
- Application doesn't see changes
- Notifier callback not invoked

**Diagnosis:**
```bash
# Check if inotify is working
inotifywait -m /etc/system.conf.d/

# Check inotify limits
cat /proc/sys/fs/inotify/max_user_watches
cat /proc/sys/fs/inotify/max_user_instances
```

**Common causes:**

1. **inotify not supported**

   Filesystem doesn't support inotify (e.g., NFS, some virtual filesystems).

   **Workaround:** Manually reload configuration:
   ```cpp
   // Recreate GlobalSettings instance to reload
   settings = GlobalSettings();
   ```

2. **inotify limit reached**
   ```bash
   # Increase limits
   sudo sysctl fs.inotify.max_user_watches=524288
   sudo sysctl fs.inotify.max_user_instances=512

   # Make permanent
   echo "fs.inotify.max_user_watches=524288" | \
       sudo tee -a /etc/sysctl.conf
   sudo sysctl -p
   ```

3. **Watching wrong directory**

   Ensure files are in watched directories:
   - `/etc/{base}.conf.d/` ✓
   - `/var/{base}.conf.d/` ✓
   - `/etc/{base}.conf` - Not watched (static file)

### Type Errors

#### Type Mismatch

**Symptom:**
```cpp
// Exception: std::bad_variant_access
int port = std::get<int>(settings.get("port"));
```

**Cause:** Setting has different type than expected.

**Solution:**
```cpp
// Check type before accessing
auto value = settings.get("port", "network", SettingValue(8080));

if (std::holds_alternative<int>(value)) {
    int port = std::get<int>(value);
} else if (std::holds_alternative<std::string>(value)) {
    // Try to convert
    std::string str = std::get<std::string>(value);
    int port = std::stoi(str);
} else {
    // Use default
    int port = 8080;
}
```

Or use visitor pattern:
```cpp
auto value = settings.get("port", "network", SettingValue(8080));

int port = std::visit([](auto&& arg) -> int {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, int>) {
        return arg;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return std::stoi(arg);
    } else if constexpr (std::is_same_v<T, double>) {
        return static_cast<int>(arg);
    } else {
        return 8080;  // default
    }
}, value);
```

### Python-Specific Issues

#### Module Not Found

**Symptom:**
```python
ImportError: No module named 'GlobalSettings'
```

**Diagnosis:**
```python
import sys
print(sys.path)
```

**Solution:**
```bash
# Find where module was installed
find /usr -name "*GlobalSettings*.so" 2>/dev/null

# Add to Python path
export PYTHONPATH=/usr/local/lib/python3.x/site-packages:$PYTHONPATH

# Or rebuild with correct Python version
cmake .. -DPYTHON_EXECUTABLE=$(which python3)
make install
```

#### Wrong Python Version

**Symptom:**
Module built for Python 3.9 but using Python 3.10.

**Solution:**
```bash
# Specify Python version during build
cmake .. -DPYTHON_EXECUTABLE=/usr/bin/python3.10
make clean
make install
```

### Configuration File Issues

#### Parse Errors

**Symptom:**
Settings not loaded correctly, some settings missing.

**Diagnosis:**
Check configuration file syntax:

```conf
# Bad: Invalid syntax
[network
port = 8080

# Bad: Missing equals
port 8080

# Bad: Invalid category
network]
port = 8080

# Good: Correct syntax
[network]
port = 8080
```

**Common mistakes:**

1. **Missing bracket**
   ```conf
   # Wrong
   [network
   port = 8080

   # Correct
   [network]
   port = 8080
   ```

2. **Typos in boolean values**
   ```conf
   # Wrong
   debug = ture  # Treated as string "ture"

   # Correct
   debug = true
   ```

3. **Unquoted strings with special characters**
   ```conf
   # Wrong (if path contains #)
   path = /opt/app#version/bin

   # Correct
   path = "/opt/app#version/bin"
   ```

#### Ownership Format Confusion

**Symptom:**
Ownership declarations not working.

**Check both formats:**

```conf
# Modern format (preferred)
@key = owner_id

# Legacy format (still supported)
key/owner = owner_id

# Both work, don't mix:
[network]
port = 8080
@port = myapp          # Use this

# OR

[network]
port = 8080
port/owner = myapp     # Or this, not both
```

### Performance Issues

#### Slow Settings Access

**Symptom:**
`get()` or `set()` operations taking too long.

**Diagnosis:**
```cpp
auto start = std::chrono::high_resolution_clock::now();
auto value = settings.get("key");
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "get() took " << duration.count() << " μs" << std::endl;
```

**Solutions:**

1. **Cache frequently accessed settings**
   ```cpp
   // Don't do this in tight loop
   for (int i = 0; i < 1000000; i++) {
       auto value = settings.get("key");  // Slow!
   }

   // Do this instead
   auto value = settings.get("key");  // Once
   for (int i = 0; i < 1000000; i++) {
       // Use cached value
   }
   ```

2. **Reduce file writes**
   ```cpp
   // Batch updates instead of frequent individual writes
   // Each set() writes to disk
   ```

#### Thread Contention

**Symptom:**
Multiple threads blocking on settings access.

**Solution:**
Implement application-level caching to reduce lock contention:

```cpp
class ConfigCache {
private:
    GlobalSettings& settings_;
    std::map<std::string, SettingValue> cache_;
    std::mutex cache_mutex_;

public:
    SettingValue getCached(const std::string& key, const std::string& category) {
        std::string full_key = category + ":" + key;

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = cache_.find(full_key);
            if (it != cache_.end()) {
                return it->second;
            }
        }

        // Cache miss - load from GlobalSettings
        auto value = settings_.get(key, category);

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cache_[full_key] = value;
        }

        return value;
    }
};
```

## Debugging Tools

### CLI Tool

Use the `settings` command for debugging:

```bash
# View all settings
settings

# Get specific setting
settings [category]key

# Set setting (bypasses ownership - use for debugging only)
settings [category]key value

# Examples
settings [network]port
settings [app]debug true
```

### Logging

Enable debug logging in your application:

```cpp
settings.setNotifier([](const std::string& cat,
                        const std::string& key,
                        SettingValue val) {
    std::cerr << "[DEBUG] Setting changed: ["
              << cat << "]" << key
              << " = " << valueToStr(val) << std::endl;
});
```

### Strace

Use `strace` to debug file access issues:

```bash
# See what files application is accessing
strace -e open,openat,read,write ./myapp 2>&1 | grep -i conf

# Example output:
# openat(AT_FDCWD, "/etc/system.conf", O_RDONLY) = 3
# openat(AT_FDCWD, "/etc/system.conf.d/", O_RDONLY) = 4
```

## Getting Help

If you're still experiencing issues:

1. **Check logs:** Look for error messages in application logs or syslog
2. **Verify installation:** Ensure library and headers are correctly installed
3. **Test with CLI tool:** Use `settings` command to verify configuration
4. **Check permissions:** Verify file and directory permissions
5. **Review examples:** Compare your code with working examples
6. **Search issues:** Check GitHub issues for similar problems
7. **Create issue:** If problem persists, create a detailed bug report

### Bug Report Template

When reporting issues, include:

- Operating system and version
- Compiler and version
- GlobalSettings version
- Configuration files (sanitized)
- Code snippet reproducing the issue
- Error messages or unexpected behavior
- Steps to reproduce

## See Also

- [Getting Started](getting-started.md) - Basic setup
- [API Reference](api-reference.md) - API documentation
- [Configuration Format](configuration.md) - File syntax
- [Examples](examples.md) - Working code examples
