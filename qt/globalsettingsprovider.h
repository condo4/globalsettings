#ifndef GLOBALSETTINGSPROVIDER_H
#define GLOBALSETTINGSPROVIDER_H

#include <QObject>
#include <globalsettings.h>
#include <memory>

class GlobalSettingsProvider : public QObject
{
    Q_OBJECT

private:
    std::unordered_map<QString, std::shared_ptr<GlobalSettings>> m_settings;
    explicit GlobalSettingsProvider(QObject *parent = nullptr);

public:
    static std::shared_ptr<GlobalSettings> get(QString id = "system");


signals:
};

#endif // GLOBALSETTINGSPROVIDER_H
