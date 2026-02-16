# GlobalSettings Qt Binding Documentation

## Overview

The GlobalSettings Qt binding provides seamless integration between the GlobalSettings configuration system and Qt/QML applications. It enables automatic property binding, change notifications, and type conversion between C++ and QML.

## Features

- **QML Property Binding**: Automatically updates QML properties when configuration files change
- **Type Conversion**: Automatic conversion between `SettingValue` and `QVariant` types
- **Thread-Safe**: All operations are protected by mutexes
- **Reactive**: File changes are detected via inotify and propagated to QML
- **Singleton Pattern**: Shared GlobalSettings instances across your application

---

## QML Usage

### Import

First, import the GlobalSettings module in your QML file:

```qml
import QtQuick
import GlobalSettings
```

### Basic Usage

The simplest way to use GlobalSettings in QML is through the `GlobalProperty` component:

```qml
Window {
    width: 640
    height: 480
    visible: true
    title: "GlobalSettings Example"

    GlobalProperty {
        id: serverPort
        key: "myapp:[Network]port"
    }

    Text {
        text: "Server Port: " + serverPort.value
        // Updates automatically when the configuration file changes!
    }
}
```

### Properties

#### `key` (QString, read/write)

The configuration key to watch. Format: `root:[category]item`

- **root**: Optional, identifies the GlobalSettings instance (default: "system")
- **category**: Optional, configuration category (default: "General")
- **item**: Required, configuration item name

**Examples:**
- `"port"` → reads `system.conf`, category "General", item "port"
- `"[Network]port"` → reads `system.conf`, category "Network", item "port"
- `"myapp:port"` → reads from `myapp.conf`, category "General", item "port"
- `"myapp:[Network]port"` → reads from `myapp.conf`, category "Network", item "port"

#### `value` (QVariant, read/write)

The current value of the setting. Automatically updates when:
- The configuration file is modified by another process
- `setValue()` is called on this property
- Another part of your application writes to the same setting

**Supported types:**
- `string` (QString in QML)
- `int`
- `double`
- `bool`

### Complete Example

With 

- /etc/fiber.conf.d/test.conf 

```
[Fiber]
release="Foo"
@release=fiber

[Network]
port=8002
@port=fiber
```

The C++ application contains:

```c++
    QGuiApplication app(argc, argv);
    app.setApplicationName("fiber");
```

The QML is

```qml
import QtQuick
import QtQuick.Controls
import GlobalSettings

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "Settings Dashboard"

    // Read-only property
    GlobalProperty {
        id: releaseVersion
        key: "fiber:[Fiber]release"
    }

    // Read-write property
    GlobalProperty {
        id: serverPort
        key: "myapp:[Network]port"
    }

    // Display current values
    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Release: " + releaseVersion.value
        }

        Row {
            spacing: 10

            Text {
                text: "Port: " + serverPort.value
            }

            // Change the port (writes to configuration file)
            SpinBox {
                from: 1024
                to: 65535
                value: serverPort.value
                onValueModified: {
                    serverPort.value = value
                }
            }
        }
    }
}
```

### Signal Notifications

`GlobalProperty` emits signals when properties change:

```qml
GlobalProperty {
    id: setting
    key: "myapp:timeout"

    onValueChanged: {
        console.log("New timeout value:", setting.value)
    }
}
```

---

## C++ Usage

### Option 1: Direct GlobalSettings Usage

For simple use cases, use `GlobalSettings` directly:

```cpp
#include <globalsettings.h>
#include <QDebug>

void example() {
    // Create a GlobalSettings instance
    GlobalSettings settings("myapp");

    // Read a value
    SettingValue value = settings.get("port", "Network", SettingValue(8080));

    if (std::holds_alternative<int>(value)) {
        int port = std::get<int>(value);
        qDebug() << "Port:" << port;
    }

    // Write a value (requires ownership marker in config file)
    bool success = settings.set("port", SettingValue(9000), "Network");
    if (success) {
        qDebug() << "Port updated successfully";
    }
}
```

### Option 2: Using GlobalSettingsProvider (Recommended)

For applications that need shared instances across components:

```cpp
#include <globalsettingsprovider.h>
#include <QDebug>

void example() {
    // Get or create a shared GlobalSettings instance
    auto settings = GlobalSettingsProvider::get("myapp");

    if (settings) {
        // Read value
        SettingValue value = settings->get("port", "Network", SettingValue(8080));

        // Write value
        settings->set("port", SettingValue(9000), "Network");
    }
}
```

**Benefits:**
- Singleton pattern: Same instance shared across your application
- Managed lifecycle: Attached to `QCoreApplication`
- Efficient: No duplicate file watchers

### Type Conversion Helpers

The binding includes helper functions for converting between `SettingValue` and `QVariant`:

```cpp
#include "globalproperty.h"  // Contains toQVariant() and fromQVariant()

// Convert SettingValue to QVariant
SettingValue setting(42);
QVariant qvar = toQVariant(setting);

// Convert QVariant to SettingValue
QVariant qvar2(QString("test"));
SettingValue setting2 = fromQVariant(qvar2);
```

### Change Notifications in C++

Register a callback to be notified when settings change:

```cpp
auto settings = GlobalSettingsProvider::get("myapp");

// Register notifier
std::size_t notifierId = settings->setNotifier(
    [](const std::string& category, const std::string& key, SettingValue value) {
        qDebug() << "Setting changed:"
                 << QString::fromStdString(category)
                 << QString::fromStdString(key);
    }
);

// Later: remove the notifier
settings->removeNotifier(notifierId);
```

### Using GlobalProperty in C++

You can also use `GlobalProperty` in C++ for automatic binding:

```cpp
#include <globalproperty.h>
#include <QObject>

class MyController : public QObject {
    Q_OBJECT

public:
    MyController(QObject* parent = nullptr) : QObject(parent) {
        m_property = new GlobalProperty("myapp:[Network]port", this);

        // Connect to value changes
        connect(m_property, &GlobalProperty::valueChanged, this, [this]() {
            qDebug() << "Port changed to:" << m_property->value();
        });
    }

private:
    GlobalProperty* m_property;
};
```

---

## Key Format Specification

### Syntax

```
root:[category]item
```

| Component | Required | Default | Description |
|-----------|----------|---------|-------------|
| `root` | No | "system" | Configuration root (determines which .conf file to read) |
| `category` | No | "General" | Configuration category (INI section) |
| `item` | Yes | - | Configuration item name (INI key) |

### Examples

| Key | Root | Category | Item | File |
|-----|------|----------|------|------|
| `"hostname"` | system | General | hostname | `/etc/system.conf` |
| `"[Network]port"` | system | Network | port | `/etc/system.conf` |
| `"myapp:version"` | myapp | General | version | `/etc/myapp.conf` |
| `"myapp:[Network]port"` | myapp | Network | port | `/etc/myapp.conf` |

### Invalid Keys

- `""` - Empty string
- `":"` - Missing item
- `"[Network"` - Unclosed bracket
- `"myapp:[Network]"` - Missing item

---

## Configuration File Format

Settings are stored in INI-style configuration files with ownership markers.

### File Locations

Configuration files are read from multiple locations in priority order:

1. `/etc/{root}.conf` - System-wide, read-only
2. `/etc/{root}.conf.d/*.conf` - System-wide fragments, read-only
3. `/var/{root}.conf.d/{appid}.conf` - Per-application, writable
4. `/tmp/{root}.conf.d/{appid}.conf` - Per-application, volatile, writable

### File Format

```ini
[General]
hostname = server01
timeout = 30

[Network]
# Ownership marker: allows 'myapp' to modify this setting
@port = myapp
port = 8080

# Volatile marker: saved in /tmp/ (cleared on reboot)
~session_id = myapp
session_id = abc123
```

**Ownership markers:**
- `@key = appid` - Marks a regular writable setting (saved in `/var/`)
- `~key = appid` - Marks a volatile setting (saved in `/tmp/`)

**Only the owning application can modify marked settings.**

### Append Operator

You can append to existing values using `+=`:

```ini
[Paths]
search_path = /usr/local/bin
search_path += /opt/bin
# Result: "/usr/local/bin /opt/bin"

[Values]
total = 10
total += 5
# Result: 15
```

**Supported for:**
- Strings (concatenated with space)
- Integers (added)
- Doubles (added)

---

## Advanced Topics

### Multiple Roots

Use different roots to separate configurations:

```qml
// System configuration
GlobalProperty {
    id: systemHostname
    key: "hostname"  // Uses default "system" root
}

// Application-specific configuration
GlobalProperty {
    id: appTheme
    key: "myapp:theme"  // Uses "myapp" root
}

// Another application's configuration
GlobalProperty {
    id: fiberRelease
    key: "fiber:[Fiber]release"  // Uses "fiber" root
}
```

### Read-Only vs Writable Settings

**Read-only** (no ownership marker):
```ini
[Network]
hostname = server01
```
- Can be read by any application
- Cannot be modified via `setValue()` or `set()`

**Writable** (with ownership marker):
```ini
[Network]
@port = myapp
port = 8080
```
- Can only be modified by the owning application (`myapp`)
- Writes go to `/var/myapp.conf.d/myapp.conf`

**Volatile** (with volatile marker):
```ini
[Session]
~session_id = myapp
session_id = xyz789
```
- Can only be modified by the owning application
- Writes go to `/tmp/myapp.conf.d/myapp.conf`
- Cleared on system reboot

### Thread Safety

All GlobalSettings operations are thread-safe:

```cpp
// Safe to call from any thread
std::thread t1([settings]() {
    settings->get("key", "Category");
});

std::thread t2([settings]() {
    settings->set("key", SettingValue(123), "Category");
});
```

**Note:** QML property bindings must be accessed from the main thread.

### Performance Considerations

**File Watching:**
- Uses inotify (Linux) for efficient file monitoring
- Only enabled when notifiers are registered
- Automatically disabled when all notifiers are removed

**Caching:**
- Settings are cached in memory
- File changes trigger automatic refresh
- No need to manually reload

**Shared Instances:**
- Use `GlobalSettingsProvider::get()` for singleton pattern
- Avoids duplicate file watchers
- Reduces memory usage

---

## Best Practices

### 1. Use Descriptive Keys

```qml
// Good
GlobalProperty {
    key: "myapp:[Network]server_port"
}

// Avoid
GlobalProperty {
    key: "myapp:p"
}
```

### 2. Group Related Settings by Category

```ini
[Network]
host = localhost
port = 8080
timeout = 30

[Database]
host = db.example.com
port = 5432
name = myapp_db
```

### 3. Use Appropriate Root Names

```qml
// System-wide settings
GlobalProperty { key: "hostname" }

// Application-specific settings
GlobalProperty { key: "myapp:[UI]theme" }

// Shared library settings
GlobalProperty { key: "fiber:[Fiber]release" }
```

### 4. Handle Missing Values

```qml
GlobalProperty {
    id: timeout
    key: "myapp:timeout"
}

// Provide fallback
Text {
    text: "Timeout: " + (timeout.value || 30) + "s"
}
```

### 5. Minimize Writable Settings

Only mark settings as writable (`@key`) if they truly need to be modified at runtime. Most settings should be read-only system configuration.

### 6. Use Volatile Settings for Temporary Data

For runtime state that shouldn't persist across reboots:

```ini
[Session]
~current_user = myapp
current_user = john_doe

~last_activity = myapp
last_activity = 1234567890
```

### 7. Cleanup Notifiers

In C++, always remove notifiers when done:

```cpp
class MyClass {
    std::shared_ptr<GlobalSettings> m_settings;
    std::size_t m_notifierId = 0;

public:
    MyClass() {
        m_settings = GlobalSettingsProvider::get("myapp");
        m_notifierId = m_settings->setNotifier([this](...) { /* ... */ });
    }

    ~MyClass() {
        if (m_settings && m_notifierId != 0) {
            m_settings->removeNotifier(m_notifierId);
        }
    }
};
```

---

## Troubleshooting

### QML: Property value doesn't update

**Check:**
1. Configuration file has correct ownership marker (`@key = appid`)
2. File permissions allow reading/writing
3. File watcher is enabled (notifiers registered)
4. Category and item names match exactly (case-sensitive)

### C++: set() returns false

**Possible causes:**
1. Setting is not marked as writable (missing `@key = appid`)
2. Application ID doesn't match ownership marker
3. File permissions deny write access
4. Invalid application ID (contains path traversal characters)

### File changes not detected

**Check:**
1. inotify is available on your system
2. Directory `/var/{root}.conf.d/` exists
3. No inotify limit reached (`/proc/sys/fs/inotify/max_user_watches`)
4. At least one notifier is registered

### Type conversion errors

**Ensure:**
1. Configuration file uses correct format (`key = value`, no quotes)
2. Numbers don't have invalid characters
3. Boolean values are `true` or `false` (lowercase)

---

## Complete Example Application

```qml
// Main.qml
import QtQuick
import QtQuick.Controls
import GlobalSettings

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "GlobalSettings Demo"

    // System hostname (read-only)
    GlobalProperty {
        id: hostname
        key: "[System]hostname"
    }

    // Application theme (writable)
    GlobalProperty {
        id: theme
        key: "myapp:[UI]theme"
    }

    // Network port (writable)
    GlobalProperty {
        id: serverPort
        key: "myapp:[Network]port"
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        // Display read-only setting
        Text {
            text: "Hostname: " + hostname.value
            font.pixelSize: 16
        }

        // Display and edit theme
        Row {
            spacing: 10
            Text {
                text: "Theme:"
                anchors.verticalCenter: parent.verticalCenter
            }
            ComboBox {
                model: ["light", "dark", "auto"]
                currentIndex: model.indexOf(theme.value)
                onActivated: {
                    theme.value = model[currentIndex]
                }
            }
        }

        // Display and edit port
        Row {
            spacing: 10
            Text {
                text: "Server Port:"
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                from: 1024
                to: 65535
                value: serverPort.value || 8080
                onValueModified: {
                    serverPort.value = value
                }
            }
        }

        // Show live updates
        Text {
            text: "Settings update automatically when files change!"
            font.italic: true
            color: "gray"
        }
    }
}
```

```ini
# /etc/myapp.conf
[UI]
theme = light
@theme = myapp

[Network]
host = localhost
port = 8080
@port = myapp

```

---

## API Reference

### QML Components

#### GlobalProperty

**Properties:**
- `key: QString` - Configuration key (read/write)
- `value: QVariant` - Setting value (read/write)

**Signals:**
- `keyChanged()` - Emitted when key changes
- `valueChanged()` - Emitted when value changes

### C++ Classes

#### GlobalSettingsProvider

```cpp
static std::shared_ptr<GlobalSettings> get(QString id = "system");
```
Returns a shared GlobalSettings instance for the given root ID.

#### GlobalSettings

```cpp
// Constructor
explicit GlobalSettings(const std::string &base = "system");

// Read value
SettingValue get(const std::string &key,
                 const std::string &category = "",
                 SettingValue def = SettingValue()) const;

// Write value
bool set(const std::string &key,
         SettingValue value,
         const std::string &category = "");

// Change notifications
std::size_t setNotifier(const std::function<void(const std::string&,
                                                   const std::string&,
                                                   SettingValue)>& callback);
void removeNotifier(std::size_t notifierId);

// Iteration
std::size_t size() const;
void forEach(const std::function<void(const SettingKey&,
                                      const SettingValue&)>& callback) const;
```

---
