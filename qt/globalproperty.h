#ifndef GLOBALPROPERTY_H
#define GLOBALPROPERTY_H

#include <QObject>
#include <QVariant>
#include <globalsettings.h>

class GlobalPropertyPrivate;

class GlobalProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)

public:
    explicit GlobalProperty(QString key = "", QObject *parent = nullptr);
    virtual ~GlobalProperty();

    QVariant value() const;
    void setValue(const QVariant &newValue);

    QString key() const;
    void setKey(const QString &newKey);

signals:
    void valueChanged();
    void keyChanged();

private:
    std::unique_ptr<GlobalPropertyPrivate> d_ptr;
    Q_DECLARE_PRIVATE(GlobalProperty)
};


static inline QVariant toQVariant(const SettingValue &value, QVariant defvalue = QVariant())
{
    if(std::holds_alternative<std::string>(value))
    {
        return QString::fromStdString(std::get<std::string>(value));
    }
    if(std::holds_alternative<int>(value))
    {
        return  std::get<int>(value);
    }
    if(std::holds_alternative<bool>(value))
    {
        return  std::get<bool>(value);
    }
    if(std::holds_alternative<double>(value))
    {
        return  std::get<double>(value);
    }
    return QVariant(defvalue);
}

static inline SettingValue fromQVariant(const QVariant &value)
{
    if(value.isNull())
        return SettingValue();

    switch (value.userType())
    {
    case QMetaType::Int:
        return value.value<int>();

    case QMetaType::QString:
        return value.toString().toStdString();

    case QMetaType::Bool:
        return value.value<bool>();

    case QMetaType::Double:
        return value.value<double>();

    default:
        return false;
    }
}
#endif // GLOBALPROPERTY_H
