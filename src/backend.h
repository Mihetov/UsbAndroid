#pragma once

#include <QObject>
#include <QThread>
#include <QVariantList>

class ModbusWorker;

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    bool connected() const;
    bool busy() const;
    QString lastError() const;

    Q_INVOKABLE void connectDevice(const QString &deviceName, int baudRate, int stopBits, const QString &parity);
    Q_INVOKABLE void readAll(int slaveId, const QVariantList &requests);
    Q_INVOKABLE void writeSelected(int slaveId, const QVariantList &requests);
    Q_INVOKABLE void writeFactoryDefaults(int slaveId, const QVariantList &requests);
    Q_INVOKABLE void close();

signals:
    void connectRequested(QString deviceName, int baudRate, int stopBits, QString parity);
    void readAllRequested(int slaveId, QVariantList requests);
    void writeRequested(int slaveId, QVariantList requests, bool rereadAfterWrite);
    void closeRequested();

    void connectedChanged();
    void busyChanged();
    void lastErrorChanged();
    void portOpened();
    void portOpenFailed(QString errorText);
    void readStarted();
    void writeStarted(QVariantList requests);
    void registerRead(QString address, QString value, bool ok, QString errorText);
    void registerWritten(QString address, bool ok, QString errorText);
    void operationFinished(QString summary, bool ok);

private slots:
    void setConnected(bool connected);
    void setBusy(bool busy);
    void setLastError(const QString &error);

private:
    QThread m_thread;
    ModbusWorker *m_worker = nullptr;
    bool m_connected = false;
    bool m_busy = false;
    QString m_lastError;
};
