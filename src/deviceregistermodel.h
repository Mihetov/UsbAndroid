#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QString>
#include <QVector>

class QJsonObject;

struct RegisterItem
{
    QString address;
    QString mnemonic;
    QString access;
    int bytes = 0;
    QString format;
    double minimum = 0;
    double maximum = 0;
    QString description;
    QString factoryValue;
    QString value;
    QString readValue;
    QString error;
    bool expanded = false;
};

class DeviceRegisterModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY hasChangesChanged)
    Q_PROPERTY(bool hasValidationErrors READ hasValidationErrors NOTIFY hasValidationErrorsChanged)

public:
    enum Roles { AddressRole = Qt::UserRole + 1, MnemonicRole, AccessRole, BytesRole, FormatRole, MinimumRole, MaximumRole, DescriptionRole, FactoryValueRole, ValueRole, ReadValueRole, ErrorRole, ExpandedRole, ChangedRole, WritableRole, ValidRole, HintRole };
    Q_ENUM(Roles)

    explicit DeviceRegisterModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    QString deviceName() const;
    bool hasChanges() const;
    bool hasValidationErrors() const;

    Q_INVOKABLE void loadModels();
    Q_INVOKABLE QVariantList availableModels() const;
    Q_INVOKABLE bool selectModel(int index);
    Q_INVOKABLE void setValue(int row, const QString &value);
    Q_INVOKABLE void toggleExpanded(int row);
    Q_INVOKABLE void readAll();
    Q_INVOKABLE QString writeChanged();
    Q_INVOKABLE void writeAll();
    Q_INVOKABLE void factoryReset();

signals:
    void deviceNameChanged();
    void hasChangesChanged();
    void hasValidationErrorsChanged();

private:
    void loadBuiltInModel();
    void loadExternalModels();
    void loadModelObject(const QJsonObject &object);
    void refreshState();
    bool isValidValue(const RegisterItem &item, QString *hint = nullptr) const;
    static bool isIntegerFormat(const QString &format);
    static bool isNumericFormat(const QString &format);

    QVector<RegisterItem> m_registers;
    QJsonArray m_models;
    QString m_deviceName;
    bool m_hasChanges = false;
    bool m_hasValidationErrors = false;
};
