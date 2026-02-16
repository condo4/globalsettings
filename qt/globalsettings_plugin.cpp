#include "globalsettings_plugin.h"
#include "globalproperty.h"

#include <qqml.h>

void GlobalSettingsPlugin::registerTypes(const char *uri)
{
    // @uri GlobalSettings
    qmlRegisterType<GlobalProperty>(uri, 1, 0, "GlobalProperty");
}
