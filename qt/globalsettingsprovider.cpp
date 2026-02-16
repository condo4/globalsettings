#include "globalsettingsprovider.h"
#include <QCoreApplication>

GlobalSettingsProvider::GlobalSettingsProvider(QObject *parent)
    : QObject{parent}
{}

std::shared_ptr<GlobalSettings> GlobalSettingsProvider::get(QString id)
{
    // Search GlobalSettingsProvider object in the QCoreApplication::instance() children
    GlobalSettingsProvider* provider = nullptr;
    QCoreApplication* app = QCoreApplication::instance();

    if (app) {
        // Search for existing provider in children
        provider = app->findChild<GlobalSettingsProvider*>(QString(), Qt::FindDirectChildrenOnly);

        // If QCoreApplication don't have any GlobalSettingsProvider child, create one with new, with QCoreApplication::instance() as parent
        if (!provider) {
            provider = new GlobalSettingsProvider(app);
        }
    } else {
        qWarning("GlobalSettingsProvider::get() - No QCoreApplication instance found");
        return nullptr;
    }

    // Search in m_settings if "id" exists
    auto it = provider->m_settings.find(id);
    if (it != provider->m_settings.end()) {
        // If exists, return the std::shared_ptr<GlobalSettings>
        return it->second;
    }

    // else create it (GlobalSettings(id.toStdString()), register in the map and return it
    auto settings = std::make_shared<GlobalSettings>(id.toStdString());
    provider->m_settings[id] = settings;
    if(!app->applicationName().isEmpty())
        settings->setId(app->applicationName().toStdString());

    return settings;
}
