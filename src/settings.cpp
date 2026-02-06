/**
 * @file settings.cpp
 * @brief Command-line interface for GlobalSettings library
 * @author GlobalSettings Team
 * @version 1.1.0
 *
 * This is a simple command-line tool for reading and writing global settings.
 * It provides a convenient interface for managing configuration from shell scripts
 * or interactive sessions.
 *
 * @section usage Usage
 * @code
 * # List all settings
 * settings
 * settings -s custom
 * settings --source custom
 *
 * # Get a specific value
 * settings hostname
 * settings [Network]hostname
 * settings -s custom hostname
 *
 * # Set a value (requires ownership)
 * settings my_key "my value"
 * settings [MyApp]port 8080
 * settings --source custom my_key "my value"
 * @endcode
 *
 * @section behavior Behavior
 * - No arguments: Lists all settings
 * - One argument: Retrieves value for the specified key
 * - Two arguments: Sets value for the specified key (if owned)
 * - Key format: [Category]key or just key (implies General category)
 * - Options:
 *   -s, --source <name>: Use custom base name (default: "system")
 *
 * @section return_codes Return Codes
 * - 0: Success
 * - 1: Error (malformed key, write failure, etc.)
 *
 * @note Requires appropriate permissions to write settings
 * @note Settings must be owned by the current application to be writable
 */

#include <iostream>
#include <globalsettings.h>
#include <utilssettings.h>

using namespace std;

/**
 * @brief Main entry point for settings command-line tool
 *
 * Provides three modes of operation:
 * 1. List all settings (no arguments)
 * 2. Get a setting value (one argument: key)
 * 3. Set a setting value (two arguments: key value)
 *
 * @param argc Argument count
 * @param argv Argument vector
 *               -s, --source <name>: Optional base name (default: "system")
 *               argv[n]: Key (optional, can include [Category] prefix)
 *               argv[n+1]: Value (optional, for set operation)
 *
 * @return 0 on success, 1 on error
 *
 * @note Categories can be embedded in the key using [Category]key syntax
 * @note Values are automatically parsed (numbers, booleans, strings)
 */
int main(int argc, char *argv[])
{
    // Check for help parameter
    if (argc >= 2) {
        std::string first_arg = argv[1];
        if (first_arg == "-h" || first_arg == "--help") {
            std::cout << "Usage: settings [OPTIONS] [KEY [VALUE]]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -s, --source <name>  Use custom base name (default: system)" << std::endl;
            std::cout << "  -h, --help           Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  settings                           # List all settings" << std::endl;
            std::cout << "  settings hostname                  # Get value for 'hostname'" << std::endl;
            std::cout << "  settings my_key \"my value\"         # Set value for 'my_key'" << std::endl;
            std::cout << "  settings [Network]hostname         # Get value with category" << std::endl;
            std::cout << "  settings -s custom hostname        # Get from custom base" << std::endl;
            std::cout << "  settings --source app my_key value # Set in custom base" << std::endl;
            return 0;
        }
    }

    // Parse command line arguments for -s/--source parameter
    std::string base = "system";  // default base
    int arg_offset = 1;  // Start processing from argv[1]

    // Check for -s or --source parameter
    if (argc >= 3) {
        std::string first_arg = argv[1];
        if (first_arg == "-s" || first_arg == "--source") {
            base = argv[2];
            arg_offset = 3;  // Skip -s and its value
        }
    }

    // Create GlobalSettings with the specified base
    GlobalSettings settings(base);

    // Calculate effective argument count after removing source parameters
    int effective_argc = argc - arg_offset + 1;

    if(effective_argc < 2)
    {
        std::cout << "Current settings (base: " << base << "):"  << std::endl;

        settings.forEach([](const SettingKey& key, const SettingValue& value) {
            std::string head;
            if(!key.first.empty())
            {
                head = "[" + key.first + "]";
            }
            std::cout << "> " << head << key.second << " = " << valueToStr(value) << std::endl;
        });
    }
    else
    {
        std::string category;
        std::string gkey = argv[arg_offset];
        if(gkey.starts_with("["))
        {
            size_t closing_bracket = gkey.find(']');
            if (closing_bracket == std::string::npos) {
                std::cerr << "ERROR: Malformed key (missing ']'): " << gkey << std::endl;
                return 1;
            }
            category = gkey.substr(1, closing_bracket - 1);
            gkey = gkey.substr(closing_bracket + 1);
        }
        if(effective_argc == 2)
        {
            std::cout << valueToStr(settings.get(gkey, category)) << std::endl;
        }
        else if(effective_argc == 3)
        {
            std::string value = argv[arg_offset + 1];
            settings.set(gkey, makeValue(value), category);
        }
    }

    return 0;
}
