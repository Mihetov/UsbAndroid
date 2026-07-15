#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QVector>

class QJsonObject;

struct RegisterEntry
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
    QString status = QStringLiteral("clean");
    QString errorMsg;
    bool expanded = false;
};

class RegisterModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY stateChanged)
    Q_PROPERTY(bool hasValidationErrors READ hasValidationErrors NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)

public:
    enum Roles {
        AddressRole = Qt::UserRole + 1,
        MnemonicRole,
        AccessRole,
        BytesRole,
        FormatRole,
        MinimumRole,
        MaximumRole,
        DescriptionRole,
        FactoryValueRole,
        ValueRole,
        ReadValueRole,
        StatusRole,
        ErrorMsgRole,
        ExpandedRole,
        ChangedRole,
        WritableRole,
        ValidRole,
        HintRole
    };
    Q_ENUM(Roles)

    explicit RegisterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    QString deviceName() const;
    bool hasChanges() const;
    bool hasValidationErrors() const;
    bool busy() const;
    void setBusy(bool busy);

    Q_INVOKABLE void reloadModels();
    Q_INVOKABLE QVariantList availableModels() const;
    Q_INVOKABLE bool selectModel(int index);
    Q_INVOKABLE void setValue(int row, const QString &value);
    Q_INVOKABLE void toggleExpanded(int row);
    Q_INVOKABLE QVariantList allReadableRequests() const;
    Q_INVOKABLE QVariantList changedWriteRequests() const;
    Q_INVOKABLE QVariantList allWriteRequests() const;
    Q_INVOKABLE QVariantList allFactoryWriteRequests() const;
    Q_INVOKABLE void applyFactoryDefaults();
    Q_INVOKABLE QVariantList singleReadRequest(int row);

public slots:
    void applyReadResult(const QString &address, const QString &value, bool ok, const QString &errorMsg);
    void applyWriteResult(const QString &address, bool ok, const QString &errorMsg);
    void markAllReading();
    void markWriting(const QVariantList &requests);

signals:
    void deviceNameChanged();
    void stateChanged();
    void busyChanged();

private:
    void loadBuiltInModel();
    void loadExternalModels();
    void loadModelObject(const QJsonObject &object);
    QVariantMap requestFor(const RegisterEntry &entry, const QString &value) const;
    int rowByAddress(const QString &address) const;
    bool isValidValue(const RegisterEntry &entry, QString *hint = nullptr) const;
    static bool isIntegerFormat(const QString &format);
    void emitRowChanged(int row);

    QVector<RegisterEntry> m_registers;
    QJsonArray m_models;
    QString m_deviceName;
    bool m_busy = false;
};
