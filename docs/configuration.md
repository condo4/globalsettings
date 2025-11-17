# Configuration File Format

## Overview

GlobalSettings uses an INI-style configuration format with support for:
- Categories (sections)
- Key-value pairs
- Multiple data types
- Ownership declarations
- Comments

## File Structure

### Basic Syntax

```conf
# Comments start with hash
# Empty lines are ignored

# Settings without category (global scope)
setting_name = value

# Category declaration
[CategoryName]
key1 = value1
key2 = value2

# Ownership declarations
@key1 = owner_id
```

## Value Types

### Strings

Strings can be quoted or unquoted:

```conf
# Quoted strings
name1 = "hello world"
name2 = 'single quotes'

# Unquoted strings
name3 = hello
name4 = hello world  # Entire line after = is the value
```

### Numbers

Integers and floating-point numbers:

```conf
# Integers
count = 42
port = 8080
negative = -100

# Floating-point
ratio = 3.14159
percent = 0.95
scientific = 1.5e-10
```

### Booleans

Multiple boolean representations:

```conf
# True values
enabled = true
active = on
debug = yes
flag = 1

# False values
disabled = false
inactive = off
production = no
flag2 = 0
```

Boolean values are case-insensitive.

## Categories

Categories organize settings into logical groups:

```conf
# Network category
[network]
host = 0.0.0.0
port = 8080
timeout = 30.0

# Database category
[database]
host = localhost
port = 5432
name = mydb

# Settings can exist without a category (global scope)
app_name = MyApplication
version = 1.0.0
```

## Ownership Declarations

Settings must declare ownership to be writable.

### Modern Format (Preferred)

```conf
[network]
port = 8080

# Declare ownership using @ prefix
@port = myapp
```

### Legacy Format (Still Supported)

```conf
[network]
port = 8080

# Legacy ownership format
port/owner = myapp
```

### Ownership Rules

1. Only the declared owner can modify a setting
2. Owner should match the executable name
3. Multiple settings can have different owners
4. Settings without ownership are read-only

**Example:**

```conf
[app]
debug = false
log_level = info
readonly_setting = production

# myapp can modify debug and log_level
@debug = myapp
@log_level = myapp

# readonly_setting has no owner - read-only
```

## File Locations

### Configuration Hierarchy

GlobalSettings reads files in this order:

1. **System defaults:** `/etc/{base}.conf`
   - System-wide default values
   - Read-only for applications

2. **Configuration fragments:** `/etc/{base}.conf.d/*.conf`
   - Configuration files from packages
   - Processed in alphabetical order
   - Read-only for applications

3. **Application settings:** `/var/{base}.conf.d/{appid}.conf`
   - Application-specific persistent data
   - Written by `set()` operations
   - One file per application

### Default Base ("system")

```
/etc/system.conf
/etc/system.conf.d/
  ├── 00-defaults.conf
  ├── 10-network.conf
  └── 20-myapp.conf
/var/system.conf.d/
  ├── myapp.conf
  └── otherapp.conf
```

### Custom Base Example ("myapp")

```cpp
GlobalSettings settings("myapp");
```

```
/etc/myapp.conf
/etc/myapp.conf.d/
  ├── defaults.conf
  └── production.conf
/var/myapp.conf.d/
  └── myapp.conf
```

## Complete Example

```conf
#
# /etc/system.conf.d/myapp.conf
# Application configuration file
#

# Global application settings
app_name = MyApplication
version = 1.0.0

# Network configuration
[network]
listen_address = 0.0.0.0
port = 8080
max_connections = 100
timeout = 30.0
keepalive = true

# Database configuration
[database]
host = localhost
port = 5432
database = myapp_db
pool_size = 10
ssl_enabled = true

# Logging configuration
[logging]
level = info
file = /var/log/myapp.log
rotate = true
max_size = 10485760  # 10 MB

# Feature flags
[features]
experimental = false
analytics = true
beta_ui = false

# Ownership declarations
# Allow myapp to modify runtime settings
@port = myapp
@timeout = myapp
@max_connections = myapp
@level = myapp
@experimental = myapp

# System settings remain read-only
# (no ownership declared for database config)
```

## Best Practices

### 1. Organization

- Use categories to group related settings
- Keep global settings minimal
- Use descriptive category names

### 2. Naming Conventions

```conf
# Good: Clear, descriptive names
[network]
connection_timeout = 30
max_retry_attempts = 3

# Avoid: Cryptic abbreviations
[net]
ct = 30
mra = 3
```

### 3. Documentation

```conf
# Add comments explaining non-obvious settings

# Connection timeout in seconds
# Increase if network is slow or unreliable
timeout = 30

# Maximum number of concurrent connections
# Higher values require more memory
max_connections = 100
```

### 4. Default Values

- Provide sensible defaults in `/etc/{base}.conf`
- Let applications override in `/var/{base}.conf.d/`
- Use application code defaults as last resort

### 5. Security

```conf
# Never store sensitive data in config files
# Use proper secret management instead

# Bad:
password = mysecret123

# Good:
secret_path = /etc/secrets/myapp.key
```

### 6. Ownership

```conf
# Only declare ownership for settings that need runtime modification
# Keep system configuration read-only

[app]
# Runtime setting - declare ownership
debug = false
@debug = myapp

# System setting - no ownership (read-only)
install_path = /opt/myapp
```

## Troubleshooting

### Setting Not Found

Check file locations and categories:
```bash
# View effective configuration
settings

# Check specific setting
settings [category]key
```

### Cannot Write Setting

Verify ownership declaration:
```conf
# Ensure ownership is declared
@your_key = your_app_name
```

### Values Not Updating

- Check file permissions on `/var/{base}.conf.d/`
- Verify application has write access
- Check for syntax errors in config files

### Type Mismatches

```cpp
// Use appropriate type conversion
auto value = settings.get("port", "network", SettingValue(8080));
if (std::holds_alternative<int>(value)) {
    int port = std::get<int>(value);
}
```
