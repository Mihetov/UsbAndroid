#include "backend.h"
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
// Добавляем системный хедер интерфейсов Android для Qt6
#include <qnativeinterface.h>
#endif
#include <QByteArray>
#include <QElapsedTimer>
#include <QJniEnvironment>
#include <QJniObject>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QVariantMap>
#include <QDebug>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "MB/modbusException.hpp"
#include "MB/modbusRequest.hpp"
#include "MB/modbusResponse.hpp"
#include "MB/modbusUtils.hpp"

namespace {
constexpr int DefaultTimeoutMs = 700;
constexpr int MaxRetries = 2;

quint16 parseAddress(const QString &text)
{
    bool ok = false;
    const auto value = text.toUShort(&ok, 0);
    return ok ? value : 0;
}

int registerCountForBytes(int bytes) { return std::max(1, (bytes + 1) / 2); }

int silentIntervalMs(int baudRate)
{
    const double bitsPerCharacter = 11.0;
    const double t35ms = 3.5 * bitsPerCharacter * 1000.0 / std::max(baudRate, 1);
    return std::max(2, static_cast<int>(std::ceil(t35ms)));
}

QByteArray toByteArray(const std::vector<uint8_t> &bytes)
{
    return QByteArray(reinterpret_cast<const char *>(bytes.data()), static_cast<qsizetype>(bytes.size()));
}

std::vector<uint8_t> toVector(const QByteArray &bytes)
{
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
}

QByteArray withCrc(const std::vector<uint8_t> &pdu)
{
    auto frame = pdu;
    const uint16_t crc = MB::utils::calculateCRC(frame);
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    return toByteArray(frame);
}

QString exceptionText(const std::vector<uint8_t> &frame)
{
    try {
        MB::ModbusException ex(frame, true);
        return QString::fromStdString(ex.toString());
    } catch (const std::exception &e) {
        return QString::fromLatin1(e.what());
    }
}

QString decodeRegisters(const QByteArray &response, const QString &format, int bytes)
{
    if (response.size() < 5) return {};
    const int byteCount = static_cast<quint8>(response.at(2));
    const QByteArray payload = response.mid(3, std::min(static_cast<qsizetype>(byteCount), response.size() - 5));
    if (format == "String" || format == "TCP56" || format == "Array") return QString::fromLatin1(payload.toHex(' ').toUpper());
    if (payload.size() >= 4 && (format == "Int32" || format == "Float")) {
        quint32 raw = (quint8(payload[0]) << 24) | (quint8(payload[1]) << 16) | (quint8(payload[2]) << 8) | quint8(payload[3]);
        if (format == "Float") { float f; static_assert(sizeof(float) == sizeof(raw)); std::memcpy(&f, &raw, sizeof(f)); return QString::number(f, 'g', 8); }
        return QString::number(static_cast<qint32>(raw));
    }
    if (payload.size() >= 2) {
        const quint16 raw = (quint8(payload[0]) << 8) | quint8(payload[1]);
        if (format == "Int16" || format == "Int8") return QString::number(static_cast<qint16>(raw));
        Q_UNUSED(bytes)
        return QString::number(raw);
    }
    return QStringLiteral("0");
}

QByteArray encodeWriteValue(const QVariantMap &request)
{
    const QString format = request.value("format").toString();
    const int bytes = request.value("bytes").toInt();
    const QString value = request.value("value").toString();
    QByteArray out;
    if (format == "String" || format == "TCP56" || format == "Array") {
        out = QByteArray::fromHex(value.toLatin1());
        if (out.isEmpty()) out = value.toLatin1();
        out.resize(bytes);
        return out;
    }
    if (format == "Float") {
        float f = value.toFloat();
        quint32 raw; std::memcpy(&raw, &f, sizeof(raw));
        out.append(char((raw >> 24) & 0xFF)); out.append(char((raw >> 16) & 0xFF)); out.append(char((raw >> 8) & 0xFF)); out.append(char(raw & 0xFF));
        return out;
    }
    qint64 number = value.toLongLong();
    for (int shift = (registerCountForBytes(bytes) * 16) - 8; shift >= 0; shift -= 8) out.append(char((number >> shift) & 0xFF));
    return out;
}
} // namespace

class AndroidUsbSerialTransport
{
public:
    bool open(const QString &deviceName, int baudRate, QString *error)
    {
#ifdef Q_OS_ANDROID
        auto *androidApp = qApp->nativeInterface<QNativeInterface::QAndroidApplication>();
        if (!androidApp) {
            *error = QStringLiteral("Failed to get QAndroidApplication interface");
            return false;
        }
        // ИСПРАВЛЕНО: Используем QJniObject вместо jobject
        QJniObject context = androidApp->context();

        const int result = QJniObject::callStaticMethod<jint>(
            "org/qtproject/example/usbandroid/UsbSerialBridge",
            "openPort",
            "(Landroid/content/Context;Ljava/lang/String;I)I",
            context.object(), // ИСПРАВЛЕНО: Передаем сырой jobject через .object()
            QJniObject::fromString(deviceName).object<jstring>(),
            baudRate);
        if (result != 0) { *error = QStringLiteral("USB openPort failed, code %1").arg(result); return false; }
#else
        Q_UNUSED(deviceName)
        Q_UNUSED(baudRate)
#endif
        m_deviceName = deviceName;
        m_baudRate = baudRate;
        m_t35Ms = silentIntervalMs(baudRate);
        return true;
    }

    QByteArray transact(const QByteArray &request, int expectedMinSize, QString *error)
    {
        QMutexLocker locker(&m_mutex);
        QThread::msleep(m_t35Ms);
#ifdef Q_OS_ANDROID
        // ВЫВОД ЗАПРОСА В ЛОГ (в верхнем регистре через пробелы)
        qDebug().noquote() << "--> MODBUS TX:" << request.toHex(' ').toUpper();

        QJniEnvironment env;
        jbyteArray bytes = env->NewByteArray(request.size());
        env->SetByteArrayRegion(bytes, 0, request.size(), reinterpret_cast<const jbyte *>(request.constData()));

        auto *androidApp = qApp->nativeInterface<QNativeInterface::QAndroidApplication>();
        if (!androidApp) {
            *error = QStringLiteral("Failed to get QAndroidApplication interface");
            env->DeleteLocalRef(bytes);
            return {};
        }
        QJniObject context = androidApp->context();

        const int written = QJniObject::callStaticMethod<jint>(
            "org/qtproject/example/usbandroid/UsbSerialBridge",
            "write",
            "(Landroid/content/Context;Ljava/lang/String;[BI)I",
            context.object(),
            QJniObject::fromString(m_deviceName).object<jstring>(),
            bytes,
            m_baudRate);
        env->DeleteLocalRef(bytes);
        if (written != request.size()) {
            *error = QStringLiteral("USB write failed, code %1").arg(written);
            qDebug() << "!!! WRITE ERROR:" << *error;
            return {};
        }

        QByteArray response;
        QElapsedTimer timer; timer.start();
        while (timer.elapsed() < DefaultTimeoutMs) {
            QJniObject data = QJniObject::callStaticObjectMethod(
                "org/qtproject/example/usbandroid/UsbSerialBridge",
                "read",
                "(II)[B",
                256,
                80
                );

            jbyteArray arr = static_cast<jbyteArray>(data.object());

            if (arr) {
                const jsize len = env->GetArrayLength(arr);
                if (len > 0) {
                    const qsizetype old = response.size();
                    response.resize(old + len);
                    env->GetByteArrayRegion(arr, 0, len, reinterpret_cast<jbyte *>(response.data() + old));
                    if (response.size() >= expectedMinSize) break;
                }
            }
        }

        if (response.isEmpty()) {
            *error = QStringLiteral("Modbus timeout after %1 ms").arg(DefaultTimeoutMs);
            // ЛОГИРУЕМ ТАЙМАУТ
            qDebug() << "Requested" << expectedMinSize << "bytes, but got absolute silence. TX/RX mismatch?";
            return {};
        }

        // ВЫВОД ОТВЕТА В ЛОГ
        qDebug().noquote() << "<-- MODBUS RX:" << response.toHex(' ').toUpper();

        QThread::msleep(m_t35Ms);
        return response;
#else
        Q_UNUSED(request)
        Q_UNUSED(expectedMinSize)
        *error = QStringLiteral("Android USB transport is available only in Android builds");
        return {};
#endif
    }

    void close()
    {
#ifdef Q_OS_ANDROID
        QJniObject::callStaticMethod<void>("org/qtproject/example/usbandroid/UsbSerialBridge", "close", "()V");
#endif
    }

private:
    QMutex m_mutex;
    QString m_deviceName;
    int m_baudRate = 115200;
    int m_t35Ms = 2;
};

class ModbusWorker : public QObject
{
    Q_OBJECT
public slots:
    void connectPort(const QString &deviceName, int baudRate, int stopBits, const QString &parity)
    {
        emit busyChanged(true);
        QString error;
        if (stopBits != 1 || parity.toLower() != QStringLiteral("none")) error = QStringLiteral("Only 8N1 is supported by the Android USB bridge");
        const bool ok = error.isEmpty() && m_transport.open(deviceName, baudRate, &error);
        emit connectedChanged(ok);
        emit lastErrorChanged(error);
        ok ? emit portOpened() : emit portOpenFailed(error);
        emit busyChanged(false);
    }

    void readAll(int slaveId, const QVariantList &requests)
    {
        emit busyChanged(true);
        emit readStarted();
        bool allOk = true;
        for (const auto &item : requests) {
            const auto req = item.toMap();
            const QString addressText = req.value("address").toString();
            QString error;
            const QByteArray response = readHolding(slaveId, parseAddress(addressText), registerCountForBytes(req.value("bytes").toInt()), &error);
            if (response.isEmpty()) { allOk = false; emit registerRead(addressText, {}, false, error); continue; }
            emit registerRead(addressText, decodeRegisters(response, req.value("format").toString(), req.value("bytes").toInt()), true, {});
        }
        emit operationFinished(allOk ? QStringLiteral("Read all completed") : QStringLiteral("Read all completed with errors"), allOk);
        emit busyChanged(false);
    }

    void write(int slaveId, const QVariantList &requests, bool rereadAfterWrite)
    {
        Q_UNUSED(rereadAfterWrite)
        emit busyChanged(true);
        emit writeStarted(requests);
        QStringList failed;
        for (const auto &item : requests) {
            const auto req = item.toMap();
            const QString addressText = req.value("address").toString();
            QString error;
            const bool ok = writeRegisters(slaveId, parseAddress(addressText), encodeWriteValue(req), &error);
            emit registerWritten(addressText, ok, error);
            if (!ok) failed << QStringLiteral("%1 %2: %3").arg(addressText, req.value("mnemonic").toString(), error);
        }
        emit operationFinished(failed.isEmpty() ? QStringLiteral("Write completed") : failed.join('\n'), failed.isEmpty());
        emit busyChanged(false);
    }

    void close() { m_transport.close(); emit connectedChanged(false); }

signals:
    void connectedChanged(bool connected);
    void busyChanged(bool busy);
    void lastErrorChanged(QString error);
    void portOpened();
    void portOpenFailed(QString errorText);
    void readStarted();
    void writeStarted(QVariantList requests);
    void registerRead(QString address, QString value, bool ok, QString errorText);
    void registerWritten(QString address, bool ok, QString errorText);
    void operationFinished(QString summary, bool ok);

private:
    QByteArray readHolding(int slaveId, quint16 address, quint16 count, QString *error)
    {
        MB::ModbusRequest request(slaveId, MB::utils::ReadAnalogOutputHoldingRegisters, address, count);
        const QByteArray response = checkedTransaction(request.toRaw(), 5 + count * 2, error);
        if (response.isEmpty()) return {};
        if (quint8(response.at(1)) != MB::utils::ReadAnalogOutputHoldingRegisters) { *error = QStringLiteral("Unexpected function code 0x%1").arg(quint8(response.at(1)), 2, 16, QLatin1Char('0')); return {}; }
        return response;
    }

    bool writeRegisters(int slaveId, quint16 address, const QByteArray &payload, QString *error)
    {
        const quint16 count = registerCountForBytes(payload.size());
        std::vector<uint8_t> raw;
        raw.push_back(slaveId);
        if (count == 1) {
            raw.push_back(MB::utils::WriteSingleAnalogOutputRegister);
            MB::utils::pushUint16(raw, address);
            raw.push_back(payload.size() > 0 ? uint8_t(payload[0]) : 0);
            raw.push_back(payload.size() > 1 ? uint8_t(payload[1]) : 0);
            return !checkedTransaction(raw, 8, error).isEmpty();
        }
        raw.push_back(MB::utils::WriteMultipleAnalogOutputHoldingRegisters);
        MB::utils::pushUint16(raw, address);
        MB::utils::pushUint16(raw, count);
        raw.push_back(static_cast<uint8_t>(count * 2));
        QByteArray padded = payload; padded.resize(count * 2);
        for (auto ch : padded) raw.push_back(static_cast<uint8_t>(ch));
        return !checkedTransaction(raw, 8, error).isEmpty();
    }

    QByteArray checkedTransaction(const std::vector<uint8_t> &rawNoCrc, int expectedMinSize, QString *error)
    {
        for (int attempt = 0; attempt <= MaxRetries; ++attempt) {
            const QByteArray response = m_transport.transact(withCrc(rawNoCrc), expectedMinSize, error);
            if (response.isEmpty()) continue;
            auto bytes = toVector(response);
            try {
                if (MB::ModbusException::exist(bytes)) { *error = exceptionText(bytes); return {}; }
                MB::ModbusResponse::fromRawCRC(bytes);
                return response;
            } catch (const std::exception &e) {
                *error = QStringLiteral("Corrupted Modbus RTU frame: %1").arg(QString::fromLatin1(e.what()));
            }
        }
        return {};
    }

    AndroidUsbSerialTransport m_transport;
};

Backend::Backend(QObject *parent) : QObject(parent), m_worker(new ModbusWorker)
{
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &Backend::connectRequested, m_worker, &ModbusWorker::connectPort);
    connect(this, &Backend::readAllRequested, m_worker, &ModbusWorker::readAll);
    connect(this, &Backend::writeRequested, m_worker, &ModbusWorker::write);
    connect(this, &Backend::closeRequested, m_worker, &ModbusWorker::close);
    connect(m_worker, &ModbusWorker::connectedChanged, this, &Backend::setConnected);
    connect(m_worker, &ModbusWorker::busyChanged, this, &Backend::setBusy);
    connect(m_worker, &ModbusWorker::lastErrorChanged, this, &Backend::setLastError);
    connect(m_worker, &ModbusWorker::portOpened, this, &Backend::portOpened);
    connect(m_worker, &ModbusWorker::portOpenFailed, this, &Backend::portOpenFailed);
    connect(m_worker, &ModbusWorker::readStarted, this, &Backend::readStarted);
    connect(m_worker, &ModbusWorker::writeStarted, this, &Backend::writeStarted);
    connect(m_worker, &ModbusWorker::registerRead, this, &Backend::registerRead);
    connect(m_worker, &ModbusWorker::registerWritten, this, &Backend::registerWritten);
    connect(m_worker, &ModbusWorker::operationFinished, this, &Backend::operationFinished);
    m_thread.start();
}

Backend::~Backend()
{
    emit closeRequested();
    m_thread.quit();
    m_thread.wait();
}

bool Backend::connected() const { return m_connected; }
bool Backend::busy() const { return m_busy; }
QString Backend::lastError() const { return m_lastError; }
void Backend::connectDevice(const QString &deviceName, int baudRate, int stopBits, const QString &parity) { emit connectRequested(deviceName, baudRate, stopBits, parity); }
void Backend::readAll(int slaveId, const QVariantList &requests) { emit readAllRequested(slaveId, requests); }
void Backend::writeSelected(int slaveId, const QVariantList &requests) { emit writeRequested(slaveId, requests, true); }
void Backend::writeFactoryDefaults(int slaveId, const QVariantList &requests) { emit writeRequested(slaveId, requests, true); }
void Backend::close() { emit closeRequested(); }
void Backend::setConnected(bool connected) { if (m_connected == connected) return; m_connected = connected; emit connectedChanged(); }
void Backend::setBusy(bool busy) { if (m_busy == busy) return; m_busy = busy; emit busyChanged(); }
void Backend::setLastError(const QString &error) { if (m_lastError == error) return; m_lastError = error; emit lastErrorChanged(); }

#include "backend.moc"
