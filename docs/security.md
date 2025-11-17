# Security Considerations

## Overview

GlobalSettings implements multiple security features to protect configuration data and prevent unauthorized modifications. This document describes the security model and best practices.

## Ownership System

### Purpose

The ownership system prevents unauthorized applications from modifying settings:

```conf
# Only 'myapp' can modify these settings
[network]
port = 8080
@port = myapp

# Anyone can read, but no one can write
[system]
install_path = /opt/myapp
# No ownership declaration = read-only
```

### How It Works

1. **Declaration**: Settings declare their owner in configuration files
2. **Verification**: GlobalSettings checks application ID against declared owner
3. **Enforcement**: set() operations fail if application is not the owner

```cpp
bool GlobalSettings::set(const std::string& key,
                         const SettingValue& value,
                         const std::string& category) {
    // Check ownership
    if (owners_.count({category, key}) > 0) {
        if (id() != owners_[{category, key}]) {
            return false;  // Not authorized
        }
    }

    // Proceed with modification
    // ...
}
```

### Instance ID Determination

The instance ID is determined from the executable name:

```cpp
// C++ application: /usr/bin/myapp
// Instance ID: "myapp"

GlobalSettings settings;
std::cout << settings.id() << std::endl;  // "myapp"
```

```python
# Python script: /usr/local/bin/myscript.py
# Instance ID: "myscript.py"

from GlobalSettings import GlobalSettings
settings = GlobalSettings()
# Instance ID automatically set from sys.argv[0]
```

### Custom Instance ID

Applications can override the default:

```cpp
GlobalSettings settings;
settings.setId("custom_id");

// Now only settings with @key = custom_id are writable
```

## File System Security

### Directory Permissions

Configuration directories should have appropriate permissions:

```bash
# System configuration (read-only for applications)
/etc/system.conf
/etc/system.conf.d/
Owner: root:root
Permissions: 644 (rw-r--r--)

# Application data (writable by applications)
/var/system.conf.d/
Owner: root:root
Permissions: 755 (rwxr-xr-x)

# Individual application files
/var/system.conf.d/myapp.conf
Owner: myapp:myapp (or root:root)
Permissions: 644 (rw-r--r--)
```

### Security Best Practices

1. **System defaults**: Keep `/etc` read-only for applications
2. **Application data**: Allow write access to `/var/system.conf.d/`
3. **File ownership**: Use appropriate user/group ownership
4. **No world-writable**: Never make configuration files world-writable

## Sensitive Data

### Don't Store Secrets in Configuration

**Bad practice:**

```conf
[database]
password = mysecretpassword123  # NEVER DO THIS
api_key = sk_live_abc123xyz     # NEVER DO THIS
```

**Good practice:**

```conf
[database]
# Reference to secret storage
password_file = /etc/secrets/db_password
# Or use environment variable
use_env_var = DB_PASSWORD

[api]
# Reference secret management system
credentials_path = /var/lib/myapp/credentials.json
```

### Secure Secret Management

```cpp
#include <fstream>
#include <globalsettings.h>

std::string loadPassword(const GlobalSettings& settings) {
    // Get path to password file from settings
    auto pathValue = settings.get("password_file", "database",
                                  SettingValue("/etc/secrets/db_password"));
    std::string path = std::get<std::string>(pathValue);

    // Read password from secure file
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot read password file");
    }

    std::string password;
    std::getline(file, password);
    return password;
}
```

### Encryption

For sensitive settings that must be stored in configuration files:

```cpp
// Encrypt sensitive values before storing
std::string encryptValue(const std::string& plaintext) {
    // Use proper encryption library (e.g., libsodium, OpenSSL)
    return encrypted_value;
}

// Store encrypted value
settings.set("encrypted_token",
             SettingValue(encryptValue(sensitive_token)),
             "app");

// Decrypt when reading
std::string encrypted = std::get<std::string>(
    settings.get("encrypted_token", "app")
);
std::string decrypted = decryptValue(encrypted);
```

## Path Traversal Prevention

### Risk

Malicious configuration could attempt path traversal:

```conf
# Malicious attempt
[app]
data_directory = ../../../../etc/passwd
```

### Mitigation

Always validate and sanitize paths:

```cpp
#include <filesystem>

std::string sanitizePath(const std::string& path) {
    namespace fs = std::filesystem;

    // Get canonical path (resolves .., symlinks, etc.)
    fs::path canonical = fs::canonical(path);

    // Verify it's within allowed directory
    fs::path allowed = fs::canonical("/var/lib/myapp");

    auto [rel, end] = std::mismatch(
        allowed.begin(), allowed.end(),
        canonical.begin(), canonical.end()
    );

    if (rel != allowed.end()) {
        throw std::runtime_error("Path traversal detected");
    }

    return canonical.string();
}

// Usage
auto pathValue = settings.get("data_directory", "app",
                              SettingValue("/var/lib/myapp/data"));
std::string safe_path = sanitizePath(std::get<std::string>(pathValue));
```

## Injection Vulnerabilities

### Command Injection

**Bad practice:**

```cpp
// NEVER execute settings directly
auto cmd = std::get<std::string>(settings.get("command"));
system(cmd.c_str());  // DANGEROUS!
```

**Good practice:**

```cpp
// Validate and sanitize input
auto cmd = std::get<std::string>(settings.get("command"));

// Whitelist allowed commands
const std::set<std::string> allowed_commands = {
    "start", "stop", "restart", "status"
};

if (allowed_commands.count(cmd) == 0) {
    throw std::runtime_error("Invalid command");
}

// Use safe execution method
executeCommand(cmd);  // Your safe implementation
```

### SQL Injection

**Bad practice:**

```cpp
// NEVER concatenate settings into SQL
auto table = std::get<std::string>(settings.get("table_name"));
std::string query = "SELECT * FROM " + table;  // VULNERABLE
```

**Good practice:**

```cpp
// Use parameterized queries or whitelist
auto table = std::get<std::string>(settings.get("table_name"));

// Whitelist validation
const std::set<std::string> valid_tables = {
    "users", "products", "orders"
};

if (valid_tables.count(table) == 0) {
    throw std::runtime_error("Invalid table name");
}

// Safe to use
std::string query = "SELECT * FROM " + table;
```

## Input Validation

### Validate All Settings

```cpp
class SettingsValidator {
public:
    static int validatePort(const SettingValue& value) {
        int port = std::get<int>(value);

        if (port < 1 || port > 65535) {
            throw std::range_error("Invalid port number");
        }

        return port;
    }

    static std::string validateHost(const SettingValue& value) {
        std::string host = std::get<std::string>(value);

        // Basic validation
        if (host.empty()) {
            throw std::invalid_argument("Host cannot be empty");
        }

        // Additional validation (hostname format, etc.)
        // ...

        return host;
    }

    static int validateTimeout(const SettingValue& value) {
        double timeout = std::get<double>(value);

        if (timeout < 0 || timeout > 3600) {
            throw std::range_error("Timeout must be between 0 and 3600");
        }

        return static_cast<int>(timeout);
    }
};

// Usage
try {
    auto portValue = settings.get("port", "network", SettingValue(8080));
    int port = SettingsValidator::validatePort(portValue);

    auto hostValue = settings.get("host", "network", SettingValue("localhost"));
    std::string host = SettingsValidator::validateHost(hostValue);
} catch (const std::exception& e) {
    std::cerr << "Invalid configuration: " << e.what() << std::endl;
    // Use safe defaults or exit
}
```

## Audit Logging

### Log Security-Relevant Events

```cpp
class AuditLogger {
public:
    static void logSettingChange(const std::string& category,
                                 const std::string& key,
                                 const SettingValue& old_value,
                                 const SettingValue& new_value,
                                 const std::string& app_id) {
        // Log to syslog or security audit log
        syslog(LOG_INFO,
               "Setting changed: [%s]%s from %s to %s by %s",
               category.c_str(),
               key.c_str(),
               valueToStr(old_value).c_str(),
               valueToStr(new_value).c_str(),
               app_id.c_str());
    }

    static void logUnauthorizedAccess(const std::string& category,
                                     const std::string& key,
                                     const std::string& app_id) {
        syslog(LOG_WARNING,
               "Unauthorized modification attempt: [%s]%s by %s",
               category.c_str(),
               key.c_str(),
               app_id.c_str());
    }
};

// Use with GlobalSettings
settings.setNotifier([](const std::string& category,
                        const std::string& key,
                        SettingValue value) {
    AuditLogger::logSettingChange(category, key, old_value, value, app_id);
});
```

## CLI Tool Security

### WARNING: Bypasses Ownership

The `settings` CLI tool **bypasses ownership restrictions**:

```bash
# This works even without ownership declaration
settings [network]port 9000
```

**Security implications:**
- Should only be used by administrators
- Requires appropriate file system permissions
- Should not be used in production automation

**Recommendations:**
1. Restrict access to root/administrators only
2. Use proper application APIs for automated changes
3. Audit CLI tool usage
4. Consider removing from production systems

## Privilege Separation

### Run with Minimal Privileges

```cpp
// Drop privileges after initialization
void dropPrivileges() {
    // Read configuration first (may need elevated privileges)
    GlobalSettings settings;
    auto config = loadConfiguration(settings);

    // Drop to unprivileged user
    if (setuid(getpwnam("myapp")->pw_uid) != 0) {
        throw std::runtime_error("Failed to drop privileges");
    }

    // Continue with reduced privileges
    runApplication(config);
}
```

## Thread Safety and Security

Thread safety prevents:
- **Race conditions**: Data corruption from concurrent access
- **TOCTOU vulnerabilities**: Time-of-check-time-of-use issues

```cpp
// Safe: Atomic check-and-set
bool secureUpdate(GlobalSettings& settings,
                  const std::string& key,
                  const SettingValue& new_value) {
    // Atomic operation - no race condition
    return settings.set(key, new_value);
}
```

## Security Checklist

Use this checklist when deploying GlobalSettings:

- [ ] Configuration files have correct ownership (root:root)
- [ ] Configuration files have correct permissions (644)
- [ ] Application directories writable only by app user
- [ ] Ownership declared for all writable settings
- [ ] No secrets stored in configuration files
- [ ] All user input validated before use
- [ ] Path traversal protection implemented
- [ ] Command injection prevention in place
- [ ] Audit logging enabled for security events
- [ ] CLI tool access restricted to administrators
- [ ] Application runs with minimal necessary privileges

## Reporting Security Issues

If you discover a security vulnerability in GlobalSettings:

1. **Do not** open a public issue
2. Contact the security team directly
3. Provide detailed reproduction steps
4. Allow reasonable time for a fix before disclosure

## See Also

- [Configuration Format](configuration.md) - Configuration file syntax
- [API Reference](api-reference.md) - API documentation
- [Architecture](architecture.md) - Internal design
