#include "globalproperty.h"
#include "globalsettingsprovider.h"
#include <globalsettings.h>
#include <QRegularExpression>


class GlobalPropertyPrivate
{
public:
    GlobalPropertyPrivate(GlobalProperty *parent)
        : q_ptr(parent)
    {
    }

    ~GlobalPropertyPrivate()
    {
        // Cleanup notifier on destruction
        if (m_settings && m_notifierId != 0) {
            m_settings->removeNotifier(m_notifierId);
        }
    }

    GlobalProperty * const q_ptr;
    std::shared_ptr<GlobalSettings> m_settings;
    QString root;
    QString category;
    QString item;
    QVariant def;
    std::size_t m_notifierId = 0;

    bool setKey(const QString &key)
    {
        if (key.isEmpty()) {
            return false;
        }

        // Pattern: ^(?:([^:\[\]]+):)?(?:\[([^\]]+)\])?(.+)$
        // Group 1: root (optional, before ':')
        // Group 2: category (optional, between [])
        // Group 3: item (required, remaining text)
        // Examples: "[foo]bar", "myapp:[foo]bar", "toto", "myapp:toto"
        static const QRegularExpression re("^(?:([^:\\[\\]]+):)?(?:\\[([^\\]]+)\\])?(.+)$");

        QRegularExpressionMatch match = re.match(key);
        if (!match.hasMatch()) {
            return false; // Invalid format
        }

        // Extract captured groups and apply defaults
        root = match.captured(1).isEmpty() ? "system" : match.captured(1);
        category = match.captured(2).isEmpty() ? "General" : match.captured(2);
        item = match.captured(3);

        // Remove previous notifier if exists
        if (m_settings && m_notifierId != 0) {
            m_settings->removeNotifier(m_notifierId);
            m_notifierId = 0;
        }

        // Get GlobalSettings instance for this root
        m_settings = GlobalSettingsProvider::get(root);

        // Register callback to receive change notifications
        if (m_settings) {
            m_notifierId = m_settings->setNotifier(
                [this](const std::string& cat, const std::string& key, SettingValue val) {
                    this->event(cat, key, val);
                }
            );
        }

        return true;
    }

    void event(const std::string& cat, const std::string& key, SettingValue)
    {
        Q_Q(GlobalProperty);

        // Filter: only react to changes for our specific category/item
        if (cat == category.toStdString() && key == item.toStdString()) {
            // Our watched property has changed, notify Qt binding system
            emit q->valueChanged();
        }
    }

    Q_DECLARE_PUBLIC(GlobalProperty)
};

GlobalProperty::GlobalProperty(QString key, QObject *parent)
    : QObject{parent}
    , d_ptr(std::make_unique<GlobalPropertyPrivate>(this))
{
    if(!key.isEmpty())
    {
        d_ptr->setKey(key);
    }
}

GlobalProperty::~GlobalProperty() = default;

QVariant GlobalProperty::value() const
{
    Q_D(const GlobalProperty);
    if(d->m_settings)
    {
        return toQVariant(d->m_settings->get(d->item.toStdString(), d->category.toStdString(), fromQVariant(d->def)));
    }
    return QVariant();
}

void GlobalProperty::setValue(const QVariant &newValue)
{
    // TODO
    Q_D(GlobalProperty);
    if(d->m_settings)
    {
        d->m_settings->set(d->item.toStdString(), fromQVariant(newValue), d->category.toStdString());
    }
    emit valueChanged();
}

QString GlobalProperty::key() const
{
    Q_D(const GlobalProperty);
    QString result;

    // Add root if not default
    if (d->root != "system") {
        result += d->root + ":";
    }

    // Add category if not default
    if (d->category != "General") {
        result += "[" + d->category + "]";
    }

    // Add item
    result += d->item;

    return result;
}

void GlobalProperty::setKey(const QString &newKey)
{
    Q_D(GlobalProperty);
    if (d->setKey(newKey)) {
        emit keyChanged();
    }
}
