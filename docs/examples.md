# Code Examples

## Table of Contents

- [Basic Usage](#basic-usage)
- [Advanced Patterns](#advanced-patterns)
- [Real-World Applications](#real-world-applications)
- [Python Examples](#python-examples)
- [Error Handling](#error-handling)

## Basic Usage

### Simple Configuration Reading

```cpp
#include <globalsettings.h>
#include <utilssettings.h>
#include <iostream>

int main() {
    GlobalSettings settings;

    // Read settings with defaults
    auto host = std::get<std::string>(
        settings.get("host", "network", SettingValue("localhost"))
    );
    auto port = std::get<int>(
        settings.get("port", "network", SettingValue(8080))
    );

    std::cout << "Server: " << host << ":" << port << std::endl;

    return 0;
}
```

### Writing Configuration

```cpp
#include <globalsettings.h>

int main() {
    GlobalSettings settings;

    // Set values (requires ownership in config file)
    bool success = settings.set("debug", SettingValue(true), "app");

    if (success) {
        std::cout << "Debug mode enabled" << std::endl;
    } else {
        std::cerr << "Failed to enable debug mode" << std::endl;
    }

    return 0;
}
```

### Using Custom Base Name

```cpp
#include <globalsettings.h>

int main() {
    // Use custom configuration base
    GlobalSettings settings("myapp");
    // Reads from /etc/myapp.conf and /var/myapp.conf.d/

    auto value = settings.get("setting", "category", SettingValue("default"));

    return 0;
}
```

## Advanced Patterns

### Configuration Class Wrapper

```cpp
#include <globalsettings.h>
#include <memory>
#include <string>

class AppConfiguration {
private:
    std::unique_ptr<GlobalSettings> settings_;

public:
    AppConfiguration(const std::string& base = "system")
        : settings_(std::make_unique<GlobalSettings>(base)) {}

    // Network configuration
    std::string getHost() const {
        return std::get<std::string>(
            settings_->get("host", "network", SettingValue("0.0.0.0"))
        );
    }

    int getPort() const {
        return std::get<int>(
            settings_->get("port", "network", SettingValue(8080))
        );
    }

    void setPort(int port) {
        settings_->set("port", SettingValue(port), "network");
    }

    // Application configuration
    bool isDebugMode() const {
        return std::get<bool>(
            settings_->get("debug", "app", SettingValue(false))
        );
    }

    void setDebugMode(bool debug) {
        settings_->set("debug", SettingValue(debug), "app");
    }

    std::string getLogLevel() const {
        return std::get<std::string>(
            settings_->get("log_level", "app", SettingValue("info"))
        );
    }

    // Database configuration
    std::string getDatabaseUrl() const {
        auto host = std::get<std::string>(
            settings_->get("host", "database", SettingValue("localhost"))
        );
        auto port = std::get<int>(
            settings_->get("port", "database", SettingValue(5432))
        );
        auto db = std::get<std::string>(
            settings_->get("database", "database", SettingValue("myapp"))
        );

        return "postgresql://" + host + ":" + std::to_string(port) + "/" + db;
    }
};

// Usage
int main() {
    AppConfiguration config("myapp");

    std::cout << "Listening on " << config.getHost()
              << ":" << config.getPort() << std::endl;
    std::cout << "Database: " << config.getDatabaseUrl() << std::endl;
    std::cout << "Debug: " << (config.isDebugMode() ? "ON" : "OFF") << std::endl;

    return 0;
}
```

### Change Notification Handler

```cpp
#include <globalsettings.h>
#include <utilssettings.h>
#include <iostream>

class ConfigurationMonitor {
private:
    GlobalSettings& settings_;

public:
    ConfigurationMonitor(GlobalSettings& settings) : settings_(settings) {
        setupNotifier();
    }

    void setupNotifier() {
        settings_.setNotifier([this](const std::string& category,
                                    const std::string& key,
                                    SettingValue value) {
            handleChange(category, key, value);
        });
    }

private:
    void handleChange(const std::string& category,
                     const std::string& key,
                     SettingValue value) {
        std::cout << "Configuration changed: ["
                  << category << "]" << key
                  << " = " << valueToStr(value) << std::endl;

        // Handle specific changes
        if (category == "app" && key == "debug") {
            bool debug = std::get<bool>(value);
            if (debug) {
                std::cout << "Debug mode enabled - increasing log verbosity" << std::endl;
                enableDebugLogging();
            } else {
                std::cout << "Debug mode disabled" << std::endl;
                disableDebugLogging();
            }
        }

        if (category == "network" && key == "port") {
            int port = std::get<int>(value);
            std::cout << "Port changed to " << port
                      << " - restart required" << std::endl;
        }
    }

    void enableDebugLogging() {
        // Implementation
    }

    void disableDebugLogging() {
        // Implementation
    }
};

int main() {
    GlobalSettings settings;
    ConfigurationMonitor monitor(settings);

    // Configuration changes will be logged automatically
    settings.set("debug", SettingValue(true), "app");

    return 0;
}
```

### Thread-Safe Configuration Cache

```cpp
#include <globalsettings.h>
#include <mutex>
#include <chrono>

class CachedConfiguration {
private:
    GlobalSettings& settings_;
    mutable std::mutex cache_mutex_;

    // Cached values
    std::string cached_host_;
    int cached_port_;
    bool cached_debug_;

    std::chrono::steady_clock::time_point last_update_;
    std::chrono::seconds cache_ttl_{60};  // 60 second TTL

public:
    CachedConfiguration(GlobalSettings& settings)
        : settings_(settings) {
        refresh();
    }

    void refresh() {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        cached_host_ = std::get<std::string>(
            settings_.get("host", "network", SettingValue("localhost"))
        );
        cached_port_ = std::get<int>(
            settings_.get("port", "network", SettingValue(8080))
        );
        cached_debug_ = std::get<bool>(
            settings_.get("debug", "app", SettingValue(false))
        );

        last_update_ = std::chrono::steady_clock::now();
    }

    std::string getHost() const {
        refreshIfNeeded();
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cached_host_;
    }

    int getPort() const {
        refreshIfNeeded();
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cached_port_;
    }

    bool isDebug() const {
        refreshIfNeeded();
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cached_debug_;
    }

private:
    void refreshIfNeeded() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_update_
        );

        if (elapsed > cache_ttl_) {
            const_cast<CachedConfiguration*>(this)->refresh();
        }
    }
};
```

### Iteration Example

```cpp
#include <globalsettings.h>
#include <utilssettings.h>
#include <iostream>

void printAllSettings(const GlobalSettings& settings) {
    std::cout << "Current configuration:" << std::endl;
    std::cout << "=====================" << std::endl;

    settings.forEach([](const SettingKey& key, const SettingValue& value) {
        const auto& [category, keyName] = key;

        if (!category.empty()) {
            std::cout << "[" << category << "]";
        }
        std::cout << keyName << " = " << valueToStr(value) << std::endl;
    });
}

int main() {
    GlobalSettings settings;
    printAllSettings(settings);

    return 0;
}
```

## Real-World Applications

### Web Server Configuration

```cpp
#include <globalsettings.h>
#include <iostream>
#include <vector>

class WebServerConfig {
private:
    GlobalSettings settings_;

public:
    WebServerConfig() : settings_("webserver") {
        // Monitor for configuration changes
        settings_.setNotifier([this](const std::string& cat,
                                    const std::string& key,
                                    SettingValue val) {
            std::cout << "Config changed: [" << cat << "]"
                      << key << std::endl;
            // Could trigger graceful reload here
        });
    }

    struct ServerConfig {
        std::string host;
        int port;
        int max_connections;
        int timeout_seconds;
        bool enable_ssl;
        std::string ssl_cert_path;
    };

    ServerConfig getServerConfig() const {
        ServerConfig config;

        config.host = std::get<std::string>(
            settings_.get("listen_address", "server", SettingValue("0.0.0.0"))
        );
        config.port = std::get<int>(
            settings_.get("port", "server", SettingValue(8080))
        );
        config.max_connections = std::get<int>(
            settings_.get("max_connections", "server", SettingValue(100))
        );
        config.timeout_seconds = std::get<int>(
            settings_.get("timeout", "server", SettingValue(30))
        );
        config.enable_ssl = std::get<bool>(
            settings_.get("enable_ssl", "ssl", SettingValue(false))
        );
        config.ssl_cert_path = std::get<std::string>(
            settings_.get("cert_path", "ssl", SettingValue("/etc/ssl/cert.pem"))
        );

        return config;
    }

    void updateMaxConnections(int max_conn) {
        settings_.set("max_connections", SettingValue(max_conn), "server");
    }
};

int main() {
    WebServerConfig config;
    auto server_config = config.getServerConfig();

    std::cout << "Starting web server:" << std::endl;
    std::cout << "  Address: " << server_config.host
              << ":" << server_config.port << std::endl;
    std::cout << "  Max connections: " << server_config.max_connections << std::endl;
    std::cout << "  SSL: " << (server_config.enable_ssl ? "enabled" : "disabled")
              << std::endl;

    // Start server with configuration...

    return 0;
}
```

### Database Connection Pool

```cpp
#include <globalsettings.h>
#include <string>

class DatabaseConfig {
private:
    GlobalSettings settings_;

public:
    DatabaseConfig() : settings_("myapp") {}

    struct ConnectionInfo {
        std::string host;
        int port;
        std::string database;
        std::string username;
        int pool_size;
        int timeout_seconds;
        bool use_ssl;
    };

    ConnectionInfo getConnectionInfo() const {
        ConnectionInfo info;

        info.host = std::get<std::string>(
            settings_.get("host", "database", SettingValue("localhost"))
        );
        info.port = std::get<int>(
            settings_.get("port", "database", SettingValue(5432))
        );
        info.database = std::get<std::string>(
            settings_.get("database", "database", SettingValue("myapp_db"))
        );
        info.username = std::get<std::string>(
            settings_.get("username", "database", SettingValue("myapp"))
        );
        info.pool_size = std::get<int>(
            settings_.get("pool_size", "database", SettingValue(10))
        );
        info.timeout_seconds = std::get<int>(
            settings_.get("timeout", "database", SettingValue(30))
        );
        info.use_ssl = std::get<bool>(
            settings_.get("use_ssl", "database", SettingValue(true))
        );

        return info;
    }

    std::string getConnectionString() const {
        auto info = getConnectionInfo();

        return "postgresql://" + info.username + "@" +
               info.host + ":" + std::to_string(info.port) +
               "/" + info.database +
               (info.use_ssl ? "?sslmode=require" : "");
    }
};
```

## Python Examples

### Flask Application Configuration

```python
from GlobalSettings import GlobalSettings
from flask import Flask

class FlaskConfig:
    def __init__(self, base="system"):
        self.settings = GlobalSettings(base)

    @property
    def HOST(self):
        return self.settings.get("host", "server", "0.0.0.0")

    @property
    def PORT(self):
        return self.settings.get("port", "server", 5000)

    @property
    def DEBUG(self):
        return self.settings.get("debug", "app", False)

    @property
    def DATABASE_URI(self):
        host = self.settings.get("host", "database", "localhost")
        port = self.settings.get("port", "database", 5432)
        db = self.settings.get("database", "database", "myapp")
        return f"postgresql://{host}:{port}/{db}"

# Create Flask app
app = Flask(__name__)
config = FlaskConfig("myapp")

@app.route('/')
def index():
    return f"Running on {config.HOST}:{config.PORT}"

if __name__ == '__main__':
    app.run(host=config.HOST, port=config.PORT, debug=config.DEBUG)
```

### Django Settings Integration

```python
from GlobalSettings import GlobalSettings

# Load GlobalSettings
_global_settings = GlobalSettings("django_app")

# Django settings
DEBUG = _global_settings.get("debug", "django", False)
SECRET_KEY = _global_settings.get("secret_key", "django", "default-secret-key")

# Database
_db_host = _global_settings.get("host", "database", "localhost")
_db_port = _global_settings.get("port", "database", 5432)
_db_name = _global_settings.get("database", "database", "django_db")
_db_user = _global_settings.get("username", "database", "django")

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.postgresql',
        'NAME': _db_name,
        'USER': _db_user,
        'HOST': _db_host,
        'PORT': _db_port,
    }
}

# Allowed hosts
ALLOWED_HOSTS = _global_settings.get("allowed_hosts", "django", "*").split(',')
```

## Error Handling

### Robust Configuration Reading

```cpp
#include <globalsettings.h>
#include <iostream>
#include <stdexcept>

class SafeConfig {
private:
    GlobalSettings settings_;

public:
    SafeConfig() : settings_("myapp") {}

    template<typename T>
    T getSafe(const std::string& key,
              const std::string& category,
              const T& defaultValue) const {
        try {
            auto value = settings_.get(key, category, SettingValue(defaultValue));

            if (std::holds_alternative<T>(value)) {
                return std::get<T>(value);
            } else {
                std::cerr << "Type mismatch for [" << category << "]"
                          << key << ", using default" << std::endl;
                return defaultValue;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading [" << category << "]" << key
                      << ": " << e.what() << std::endl;
            return defaultValue;
        }
    }

    bool setSafe(const std::string& key,
                const SettingValue& value,
                const std::string& category) {
        try {
            bool result = settings_.set(key, value, category);
            if (!result) {
                std::cerr << "Failed to set [" << category << "]"
                          << key << " (no ownership?)" << std::endl;
            }
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Error writing [" << category << "]" << key
                      << ": " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    SafeConfig config;

    // Safe reads with type checking
    int port = config.getSafe<int>("port", "network", 8080);
    std::string host = config.getSafe<std::string>("host", "network", "localhost");
    bool debug = config.getSafe<bool>("debug", "app", false);

    std::cout << "Configuration loaded: " << host << ":" << port << std::endl;

    // Safe write with error checking
    if (config.setSafe("port", SettingValue(9000), "network")) {
        std::cout << "Port updated successfully" << std::endl;
    }

    return 0;
}
```

## See Also

- [API Reference](api-reference.md) - Complete API documentation
- [Python Bindings](python-bindings.md) - Python-specific examples
- [Configuration Format](configuration.md) - Configuration file syntax
