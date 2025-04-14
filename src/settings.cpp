#include <iostream>
#include <algorithm>
#include <globalsettings.h>
#include <utilssettings.h>

using namespace std;

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
            category = gkey.substr(1, gkey.find(']') - 1);
            gkey = gkey.substr(gkey.find(']') + 1);
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
