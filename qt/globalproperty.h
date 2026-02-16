#ifndef GLOBALPROPERTY_H
#define GLOBALPROPERTY_H

#include <QObject>
#include <QVariant>

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

#endif // GLOBALPROPERTY_H
