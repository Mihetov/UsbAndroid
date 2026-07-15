#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QJniObject>
#include <QList>
#include <QObject>
#include <QString>

struct UsbEndpointInfo
{
    int address = 0;
    int number = 0;
    QString direction;
    QString type;
    int pollInterval = 0;
    int maxPacketSize = 0;
    int attributes = 0;
};

struct UsbInterfaceInfo
{
    int number = 0;
    QList<UsbEndpointInfo> endpoints;
};

struct UsbDeviceInfo
{
    QString deviceName;
    QString deviceClass;
    int vendorId = 0;
    int productId = 0;
    QString productName;
    QList<UsbInterfaceInfo> interfaces;
    QString details;
    QJniObject javaDevice;
};

class UsbScanner : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    enum Roles {
        DevicePathRole = Qt::UserRole + 1,
        DeviceClassRole,
        VendorIdRole,
        ProductIdRole,
        ProductNameRole,
        DetailsRole
    };
    Q_ENUM(Roles)

    explicit UsbScanner(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString status() const;

    Q_INVOKABLE void scan();
    Q_INVOKABLE void sendTestPacket(int row);
    Q_INVOKABLE bool connectDevice(int row);
    Q_INVOKABLE bool openPort(int baudRate, int stopBits, const QString &parity);
    Q_INVOKABLE QString lastError() const;

signals:
    void statusChanged();

private:
    void setStatus(const QString &status);
    static QString usbClassName(int deviceClass);
    static QString endpointDirection(int direction);
    static QString endpointType(int type);
    static quint16 modbusCrc(const QByteArray &payload);
    static QByteArray makeTestPacket();

    QList<UsbDeviceInfo> m_devices;
    QString m_status;
    QString m_selectedDeviceName;
    QString m_lastError;
};
