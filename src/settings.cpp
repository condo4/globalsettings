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
 *
 * # Get a specific value
 * settings hostname
 * settings [Network]hostname
 *
 * # Set a value (requires ownership)
 * settings my_key "my value"
 * settings [MyApp]port 8080
 * @endcode
 *
 * @section behavior Behavior
 * - No arguments: Lists all settings
 * - One argument: Retrieves value for the specified key
 * - Two arguments: Sets value for the specified key (if owned)
 * - Key format: [Category]key or just key (implies General category)
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
 *               argv[1]: Key (optional, can include [Category] prefix)
 *               argv[2]: Value (optional, for set operation)
 *
 * @return 0 on success, 1 on error
 *
 * @note Categories can be embedded in the key using [Category]key syntax
 * @note Values are automatically parsed (numbers, booleans, strings)
 */
int main(int argc, char *argv[])
{
    GlobalSettings settings;

    if(argc < 2)
    {
        std::cout << "Current settings:"  << std::endl;

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
        std::string gkey = argv[1];
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
        if(argc == 2)
        {
            std::cout << valueToStr(settings.get(gkey, category)) << std::endl;
        }
        else if(argc == 3)
        {
            std::string value = argv[2];
            settings.set(gkey, makeValue(value), category);
        }
    }

    return 0;
}
