# API Reference

## Core Classes

### GlobalSettings

Thread-safe global configuration settings manager.

#### Constructors

```cpp
GlobalSettings()
```
Creates a GlobalSettings instance with default base name "system".
- Configuration paths: `/etc/system.conf`, `/var/system.conf.d/`
- Instance ID: Automatically determined from executable name

```cpp
explicit GlobalSettings(const std::string& base)
```
Creates a GlobalSettings instance with custom base name.
- **Parameters:**
  - `base`: Base name for configuration files
- Configuration paths: `/etc/{base}.conf`, `/var/{base}.conf.d/`

#### Methods

##### get()

```cpp
SettingValue get(const std::string& key,
                 const std::string& category = "",
                 const SettingValue& defaultValue = SettingValue()) const
```

Retrieves a setting value.

- **Parameters:**
  - `key`: Setting key name
  - `category`: Optional category name (default: "")
  - `defaultValue`: Value to return if setting not found
- **Returns:** SettingValue (variant of string, int, double, bool, or vector\<string\>)
- **Thread-safe:** Yes

**Examples:**
```cpp
// Get with category and default
auto value = settings.get("port", "network", SettingValue(8080));
int port = std::get<int>(value);

// Get without category
auto debug = settings.get("debug", "", SettingValue(false));

// Get using category notation
auto host = settings.get("[network]host", "", SettingValue("localhost"));
```

##### set()

```cpp
bool set(const std::string& key,
         const SettingValue& value,
         const std::string& category = "")
```

Sets a setting value.

- **Parameters:**
  - `key`: Setting key name
  - `value`: Value to set
  - `category`: Optional category name (default: "")
- **Returns:** true on success, false on failure
- **Thread-safe:** Yes
- **Requires:** Ownership declaration in configuration file

**Examples:**
```cpp
// Set with category
settings.set("port", SettingValue(9000), "network");

// Set without category
settings.set("debug", SettingValue(true));

// Set using category notation
settings.set("[network]timeout", SettingValue(60.0));
```

##### setNotifier()

```cpp
void setNotifier(std::function<void(const std::string&,
                                    const std::string&,
                                    SettingValue)> notifier)
```

Sets a callback function for setting changes.

- **Parameters:**
  - `notifier`: Callback function with signature `void(category, key, value)`
- **Thread-safe:** Yes

**Example:**
```cpp
settings.setNotifier([](const std::string& category,
                       const std::string& key,
                       SettingValue value) {
    std::cout << "Changed: [" << category << "]"
              << key << " = " << valueToStr(value) << std::endl;
});
```

##### forEach()

```cpp
void forEach(std::function<void(const SettingKey&,
                               const SettingValue&)> callback) const
```

Iterates over all settings safely.

- **Parameters:**
  - `callback`: Function called for each setting
- **Thread-safe:** Yes

**Example:**
```cpp
settings.forEach([](const SettingKey& key, const SettingValue& value) {
    std::string category = key.first;
    std::string keyName = key.second;
    std::cout << "[" << category << "]" << keyName
              << " = " << valueToStr(value) << std::endl;
});
```

##### size()

```cpp
size_t size() const
```

Returns the number of settings.

- **Returns:** Count of all settings
- **Thread-safe:** Yes

##### id()

```cpp
std::string id() const
```

Gets the current instance identifier.

- **Returns:** Instance ID string
- **Thread-safe:** Yes

##### setId()

```cpp
void setId(const std::string& id)
```

Sets a custom instance identifier.

- **Parameters:**
  - `id`: New instance identifier
- **Thread-safe:** Yes

## Type Definitions

### SettingValue

```cpp
using SettingValue = std::variant<std::string, int, double, bool, std::vector<std::string>>
```

Variant type representing possible setting values.

### SettingKey

```cpp
using SettingKey = std::pair<std::string, std::string>
```

Pair representing a setting's location:
- `first`: Category name (empty string for no category)
- `second`: Key name

## Utility Functions (utilssettings.h)

### makeValue()

```cpp
SettingValue makeValue(const std::string& str)
```

Converts a string to the appropriate SettingValue type.

- **Parameters:**
  - `str`: String to convert
- **Returns:** SettingValue with appropriate type (bool, int, double, string, or vector\<string\>)

**Examples:**
```cpp
auto v1 = makeValue("42");                      // SettingValue(int)
auto v2 = makeValue("3.14");                    // SettingValue(double)
auto v3 = makeValue("true");                    // SettingValue(bool)
auto v4 = makeValue("hello");                   // SettingValue(string)
auto v5 = makeValue("\"a\", \"b\", \"c\"");    // SettingValue(vector<string>{"a","b","c"})
```

### valueToStr()

```cpp
std::string valueToStr(const SettingValue& value)
```

Converts a SettingValue to string representation.

- **Parameters:**
  - `value`: SettingValue to convert
- **Returns:** String representation

**Examples:**
```cpp
std::cout << valueToStr(SettingValue(42)) << std::endl;                              // "42"
std::cout << valueToStr(SettingValue(true)) << std::endl;                            // "true"
std::cout << valueToStr(SettingValue(3.14)) << std::endl;                            // "3.140000"
std::cout << valueToStr(SettingValue("text")) << std::endl;                          // "text"
std::cout << valueToStr(SettingValue(std::vector<std::string>{"a","b"})) << std::endl; // "\"a\", \"b\""
```

### valueToInt()

```cpp
int valueToInt(const SettingValue& value)
```

Converts a SettingValue to integer.

- **Parameters:**
  - `value`: SettingValue to convert
- **Returns:** Integer value

Conversion rules:
- `int`: Returns as-is
- `double`: Truncates to int
- `bool`: Returns 1 for true, 0 for false
- `string`: Attempts to parse as integer
- `vector<string>`: Returns the number of elements

## Error Handling

All methods provide proper error handling:
- `get()` returns the default value if key not found
- `set()` returns false if write fails (e.g., no ownership)
- File I/O errors are logged but don't crash the application

## Thread Safety Guarantees

All public methods are thread-safe:
- Multiple threads can call `get()` simultaneously
- Multiple threads can call `set()` simultaneously
- `forEach()` uses internal locking for safe iteration
- Notifier callbacks are synchronized
