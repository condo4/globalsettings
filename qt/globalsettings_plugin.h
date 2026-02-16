#ifndef GLOBALSETTINGS_PLUGIN_H
#define GLOBALSETTINGS_PLUGIN_H

#include <QQmlExtensionPlugin>

class GlobalSettingsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // GLOBALSETTINGS_PLUGIN_H
