#include "../src/globalsettings.h"
#include "../src/utilssettings.h"
#include "test_framework.h"
#include <fstream>
#include <filesystem>
#include <unistd.h>

// Test reading from /etc (read-only configs)
bool test_read_system_config() {
    GlobalSettings settings;

    // Should be able to read from /etc/system.conf
    // Note: "General" category is stored as empty string ""
    auto system_name = settings.get("system_name", "", SettingValue("not_found"));
    auto hostname = settings.get("hostname", "Network", SettingValue("not_found"));

    // Verify values from test config
    ASSERT_EQ(valueToStr(system_name), "test_system");
    ASSERT_EQ(valueToStr(hostname), "test-host");

    return true;
}

// Test reading from /etc/system.conf.d/
bool test_read_system_config_dir() {
    GlobalSettings settings;

    // Should read from /etc/system.conf.d/base.conf
    // Note: "General" category is stored as empty string ""
    auto version = settings.get("version", "", SettingValue("not_found"));
    auto environment = settings.get("environment", "", SettingValue("not_found"));

    // Note: "1.0" is parsed as double, so it becomes "1.000000"
    ASSERT_EQ(valueToStr(version), "1.000000");
    ASSERT_EQ(valueToStr(environment), "test");

    return true;
}

// Test reading from /var (writable configs)
bool test_read_var_config() {
    GlobalSettings settings;

    // Should read from /var/system.conf.d/test_functional.conf
    // Note: "General" category is stored as empty string ""
    auto app_value = settings.get("app_key", "", SettingValue("not_found"));

    ASSERT_EQ(valueToStr(app_value), "test_value");

    return true;
}

// Test writing to /var
bool test_write_var_config() {
    GlobalSettings settings;

    // The binary name is "test_functional", which is the default ID
    // No need to setId(), it should already match the owner in config

    // Try to write a new value
    bool success = settings.set("test_setting", SettingValue("new_value"), "Settings");

    // Should succeed (we own this setting)
    ASSERT(success);

    // Verify the value was written
    auto read_value = settings.get("test_setting", "Settings", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(read_value), "new_value");

    return true;
}

// Test atomic file writes
bool test_atomic_write() {
    GlobalSettings settings;
    // Using default ID from binary name (test_functional)

    // Write a value
    bool success = settings.set("test_setting", SettingValue("atomic_test"), "Settings");
    ASSERT(success);

    // Verify file exists (will be test_functional.conf)
    std::string config_path = "/var/system.conf.d/test_functional.conf";
    ASSERT(std::filesystem::exists(config_path));

    // Verify no temp files left
    for (const auto& entry : std::filesystem::directory_iterator("/var/system.conf.d")) {
        std::string filename = entry.path().filename().string();
        ASSERT(filename.find(".tmp.") == std::string::npos);
    }

    return true;
}

// Test file write permissions
bool test_write_permissions() {
    GlobalSettings settings;

    // Try to write with wrong ID
    settings.setId("wrong_app");

    // Should fail (we don't own the setting)
    bool success = settings.set("test_setting", SettingValue("should_fail"), "Settings");

    // Should NOT succeed
    ASSERT(!success);

    return true;
}

// Test multiple write/read cycles
bool test_multiple_write_read() {
    GlobalSettings settings;
    // Using default ID from binary name (test_functional)

    // Write multiple values
    for (int i = 0; i < 10; i++) {
        std::string value = "iteration_" + std::to_string(i);
        bool success = settings.set("test_setting", SettingValue(value), "Settings");
        ASSERT(success);

        // Read back immediately
        auto read_value = settings.get("test_setting", "Settings", SettingValue("not_found"));
        ASSERT_EQ(valueToStr(read_value), value);
    }

    return true;
}

// Test file corruption resistance (atomic writes)
bool test_file_corruption_resistance() {
    GlobalSettings settings;
    // Using default ID from binary name (test_functional)

    // Get original content
    auto original = settings.get("test_setting", "Settings", SettingValue("original"));

    // Write a new value
    bool success = settings.set("test_setting", SettingValue("corruption_test"), "Settings");
    ASSERT(success);

    // Verify file is valid (can be read again)
    GlobalSettings settings2;
    auto new_value = settings2.get("test_setting", "Settings", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(new_value), "corruption_test");

    return true;
}

// Test reading with no existing /var config
bool test_missing_var_config() {
    // Create settings with ID that doesn't have a config file
    GlobalSettings settings;
    settings.setId("nonexistent_app");

    // Should not crash, should return default
    auto value = settings.get("nonexistent_key", "", SettingValue("default"));
    ASSERT_EQ(valueToStr(value), "default");

    return true;
}

// Test that we're actually in namespace (can write to /var)
bool test_namespace_verification() {
    // Verify we can write to /var
    std::string test_file = "/var/namespace_test.txt";
    std::ofstream file(test_file);
    ASSERT(file.is_open());

    file << "namespace test";
    file.close();

    // Verify we can read it back
    ASSERT(std::filesystem::exists(test_file));

    // Clean up
    std::filesystem::remove(test_file);

    return true;
}

// Test configuration overlay (/etc + /var)
bool test_config_overlay() {
    GlobalSettings settings;

    // Should read from /etc (base config)
    // Note: "General" category is stored as empty string ""
    auto version = settings.get("version", "", SettingValue("not_found"));
    // Note: "1.0" is parsed as double, so it becomes "1.000000"
    ASSERT_EQ(valueToStr(version), "1.000000");

    // Should read from /var (overlay config)
    auto app_key = settings.get("app_key", "", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(app_key), "test_value");

    // Both should work simultaneously
    ASSERT(settings.size() > 0);

    return true;
}

// Test file existence checks
bool test_file_existence() {
    // Verify test files exist
    ASSERT(std::filesystem::exists("/etc/system.conf"));
    ASSERT(std::filesystem::exists("/etc/system.conf.d"));
    ASSERT(std::filesystem::exists("/etc/system.conf.d/base.conf"));
    ASSERT(std::filesystem::exists("/var/system.conf.d"));
    ASSERT(std::filesystem::exists("/var/system.conf.d/test_functional.conf"));

    return true;
}

// Test directory permissions
bool test_directory_permissions() {
    // Should be able to read /etc
    ASSERT(std::filesystem::is_directory("/etc"));
    ASSERT(std::filesystem::is_directory("/etc/system.conf.d"));

    // Should be able to write to /var
    ASSERT(std::filesystem::is_directory("/var"));
    ASSERT(std::filesystem::is_directory("/var/system.conf.d"));

    // Test we can create files in /var
    std::string test_file = "/var/system.conf.d/permission_test.txt";
    std::ofstream file(test_file);
    ASSERT(file.is_open());
    file << "test";
    file.close();

    // Clean up
    std::filesystem::remove(test_file);

    return true;
}

// Test reading from /tmp (volatile variables)
bool test_read_volatile_config() {
    GlobalSettings settings;

    // Should read from /tmp/system.conf.d/test_functional.conf
    auto volatile_value = settings.get("volatile_key", "", SettingValue("not_found"));

    ASSERT_EQ(valueToStr(volatile_value), "volatile_test_value");

    return true;
}

// Test writing to /tmp (volatile variables)
bool test_write_volatile_config() {
    GlobalSettings settings;

    // The binary name is "test_functional", which is the default ID
    // Try to write a new value to a volatile variable
    bool success = settings.set("test_volatile_setting", SettingValue("new_volatile_value"), "VolatileSettings");

    // Should succeed (we own this volatile setting)
    ASSERT(success);

    // Verify the value was written
    auto read_value = settings.get("test_volatile_setting", "VolatileSettings", SettingValue("not_found"));
    ASSERT_EQ(valueToStr(read_value), "new_volatile_value");

    return true;
}

// Test atomic file writes for volatile variables
bool test_volatile_atomic_write() {
    GlobalSettings settings;
    // Using default ID from binary name (test_functional)

    // Write a value to volatile variable
    bool success = settings.set("test_volatile_setting", SettingValue("volatile_atomic_test"), "VolatileSettings");
    ASSERT(success);

    // Verify file exists in /tmp/ (will be test_functional.conf)
    std::string config_path = "/tmp/system.conf.d/test_functional.conf";
    ASSERT(std::filesystem::exists(config_path));

    // Verify no temp files left in /tmp/
    for (const auto& entry : std::filesystem::directory_iterator("/tmp/system.conf.d")) {
        std::string filename = entry.path().filename().string();
        ASSERT(filename.find(".tmp.") == std::string::npos);
    }

    return true;
}

// Test volatile variable write permissions
bool test_volatile_write_permissions() {
    GlobalSettings settings;

    // Try to write with wrong ID
    settings.setId("wrong_app");

    // Should fail (we don't own the volatile setting)
    bool success = settings.set("test_volatile_setting", SettingValue("should_fail"), "VolatileSettings");

    // Should NOT succeed
    ASSERT(!success);

    return true;
}

// Test that volatile and regular variables are independent
bool test_volatile_independence() {
    GlobalSettings settings;

    // Write to regular variable
    bool success1 = settings.set("test_setting", SettingValue("regular_value"), "Settings");
    ASSERT(success1);

    // Write to volatile variable
    bool success2 = settings.set("test_volatile_setting", SettingValue("volatile_value"), "VolatileSettings");
    ASSERT(success2);

    // Verify regular variable is in /var/
    std::string regular_path = "/var/system.conf.d/test_functional.conf";
    ASSERT(std::filesystem::exists(regular_path));

    // Verify volatile variable is in /tmp/
    std::string volatile_path = "/tmp/system.conf.d/test_functional.conf";
    ASSERT(std::filesystem::exists(volatile_path));

    // Read back both values
    auto regular_value = settings.get("test_setting", "Settings", SettingValue("not_found"));
    auto volatile_value = settings.get("test_volatile_setting", "VolatileSettings", SettingValue("not_found"));

    ASSERT_EQ(valueToStr(regular_value), "regular_value");
    ASSERT_EQ(valueToStr(volatile_value), "volatile_value");

    return true;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   GlobalSettings Functional Tests (Namespace Isolated)  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Verify we're in namespace
    std::cout << "Verifying namespace environment..." << std::endl;
    std::cout << "  /etc -> " << (std::filesystem::exists("/etc/system.conf") ? "✅ Ready" : "❌ Missing") << std::endl;
    std::cout << "  /var -> " << (std::filesystem::exists("/var/system.conf.d") ? "✅ Ready" : "❌ Missing") << std::endl;
    std::cout << std::endl;

    TestFramework framework;

    std::cout << ">>> ENVIRONMENT VERIFICATION" << std::endl;
    framework.add_test("Namespace verification", test_namespace_verification);
    framework.add_test("File existence checks", test_file_existence);
    framework.add_test("Directory permissions", test_directory_permissions);

    std::cout << std::endl << ">>> FILE READ TESTS" << std::endl;
    framework.add_test("Read system config (/etc)", test_read_system_config);
    framework.add_test("Read system config.d (/etc)", test_read_system_config_dir);
    framework.add_test("Read var config (/var)", test_read_var_config);
    framework.add_test("Configuration overlay", test_config_overlay);
    framework.add_test("Missing var config", test_missing_var_config);

    std::cout << std::endl << ">>> FILE WRITE TESTS" << std::endl;
    framework.add_test("Write to /var", test_write_var_config);
    framework.add_test("Atomic file writes", test_atomic_write);
    framework.add_test("Write permissions check", test_write_permissions);
    framework.add_test("Multiple write/read cycles", test_multiple_write_read);
    framework.add_test("File corruption resistance", test_file_corruption_resistance);

    std::cout << std::endl << ">>> VOLATILE VARIABLE TESTS" << std::endl;
    framework.add_test("Read volatile config (/tmp)", test_read_volatile_config);
    framework.add_test("Write to /tmp", test_write_volatile_config);
    framework.add_test("Volatile atomic file writes", test_volatile_atomic_write);
    framework.add_test("Volatile write permissions check", test_volatile_write_permissions);
    framework.add_test("Volatile/Regular independence", test_volatile_independence);

    std::cout << std::endl;
    bool success = framework.run_all();

    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    if (success) {
        std::cout << "║          ✅ ALL FUNCTIONAL TESTS PASSED! ✅             ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  File read/write operations work correctly              ║" << std::endl;
        std::cout << "║  Tests ran in isolated namespace (no root needed)       ║" << std::endl;
    } else {
        std::cout << "║          ❌ SOME FUNCTIONAL TESTS FAILED! ❌            ║" << std::endl;
    }
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;

    return success ? 0 : 1;
}
