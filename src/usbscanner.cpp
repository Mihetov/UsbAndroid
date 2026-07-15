#include "usbscanner.h"

#include <QCoreApplication>
#include <QJniEnvironment>

namespace {
constexpr int UsbEndpointXferControl = 0;
constexpr int UsbEndpointXferIsochronous = 1;
constexpr int UsbEndpointXferBulk = 2;
constexpr int UsbEndpointXferInterrupt = 3;
constexpr int UsbDirOut = 0;
constexpr int UsbDirIn = 0x80;
}

UsbScanner::UsbScanner(QObject *parent)
    : QAbstractListModel(parent)
{
    setStatus(QStringLiteral("Нажмите «Сканировать USB», чтобы найти OTG устройства."));
}

int UsbScanner::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_devices.size();
}

QVariant UsbScanner::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size())
        return {};

    const auto &device = m_devices.at(index.row());
    switch (role) {
    case DevicePathRole: return device.deviceName;
    case DeviceClassRole: return device.deviceClass;
    case VendorIdRole: return QStringLiteral("0x%1").arg(device.vendorId, 4, 16, QLatin1Char('0')).toUpper();
    case ProductIdRole: return QStringLiteral("0x%1").arg(device.productId, 4, 16, QLatin1Char('0')).toUpper();
    case ProductNameRole: return device.productName;
    case DetailsRole: return device.details;
    default: return {};
    }
}

QHash<int, QByteArray> UsbScanner::roleNames() const
{
    return {{DevicePathRole, "devicePath"}, {DeviceClassRole, "deviceClass"},
            {VendorIdRole, "vendorId"}, {ProductIdRole, "productId"},
            {ProductNameRole, "productName"}, {DetailsRole, "details"}};
}

QString UsbScanner::status() const { return m_status; }

void UsbScanner::scan()
{
    beginResetModel();
    m_devices.clear();

#ifdef Q_OS_ANDROID
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject serviceName = QJniObject::fromString("usb");
    QJniObject usbManager = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", serviceName.object<jstring>());
    QJniObject deviceMap = usbManager.callObjectMethod("getDeviceList", "()Ljava/util/HashMap;");
    QJniObject values = deviceMap.callObjectMethod("values", "()Ljava/util/Collection;");
    QJniObject iterator = values.callObjectMethod("iterator", "()Ljava/util/Iterator;");

    while (iterator.callMethod<jboolean>("hasNext", "()Z")) {
        QJniObject javaDevice = iterator.callObjectMethod("next", "()Ljava/lang/Object;");
        UsbDeviceInfo info;
        info.javaDevice = javaDevice;
        info.deviceName = javaDevice.callObjectMethod("getDeviceName", "()Ljava/lang/String;").toString();
        const int cls = javaDevice.callMethod<jint>("getDeviceClass", "()I");
        info.deviceClass = usbClassName(cls);
        info.vendorId = javaDevice.callMethod<jint>("getVendorId", "()I");
        info.productId = javaDevice.callMethod<jint>("getProductId", "()I");
        info.productName = javaDevice.callObjectMethod("getProductName", "()Ljava/lang/String;").toString();
        if (info.productName.isEmpty())
            info.productName = QStringLiteral("Unknown product");

        QStringList lines;
        const int interfaceCount = javaDevice.callMethod<jint>("getInterfaceCount", "()I");
        for (int i = 0; i < interfaceCount; ++i) {
            QJniObject iface = javaDevice.callObjectMethod("getInterface", "(I)Landroid/hardware/usb/UsbInterface;", i);
            UsbInterfaceInfo ifaceInfo;
            ifaceInfo.number = iface.callMethod<jint>("getId", "()I");
            lines << QStringLiteral("Interface number: %1").arg(ifaceInfo.number);
            const int endpointCount = iface.callMethod<jint>("getEndpointCount", "()I");
            for (int e = 0; e < endpointCount; ++e) {
                QJniObject endpoint = iface.callObjectMethod("getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;", e);
                UsbEndpointInfo endpointInfo;
                endpointInfo.address = endpoint.callMethod<jint>("getAddress", "()I");
                endpointInfo.number = endpoint.callMethod<jint>("getEndpointNumber", "()I");
                endpointInfo.direction = endpointDirection(endpoint.callMethod<jint>("getDirection", "()I"));
                endpointInfo.type = endpointType(endpoint.callMethod<jint>("getType", "()I"));
                endpointInfo.pollInterval = endpoint.callMethod<jint>("getInterval", "()I");
                endpointInfo.maxPacketSize = endpoint.callMethod<jint>("getMaxPacketSize", "()I");
                endpointInfo.attributes = endpoint.callMethod<jint>("getAttributes", "()I");
                ifaceInfo.endpoints << endpointInfo;
                lines << QStringLiteral("  Endpoint %1: address=0x%2, number=%3, direction=%4, type=%5, poll interval=%6, max packet size=%7, attributes=0x%8")
                             .arg(e)
                             .arg(endpointInfo.address, 2, 16, QLatin1Char('0')).arg(endpointInfo.number)
                             .arg(endpointInfo.direction, endpointInfo.type).arg(endpointInfo.pollInterval)
                             .arg(endpointInfo.maxPacketSize).arg(endpointInfo.attributes, 2, 16, QLatin1Char('0'));
            }
            info.interfaces << ifaceInfo;
        }
        info.details = lines.join('\n');
        m_devices << info;
    }
#else
    setStatus(QStringLiteral("USB Host сканирование доступно только в Android сборке."));
#endif

    endResetModel();
    setStatus(QStringLiteral("Найдено USB устройств: %1").arg(m_devices.size()));
}

void UsbScanner::sendTestPacket(int row)
{
    if (row < 0 || row >= m_devices.size()) {
        setStatus(QStringLiteral("Устройство не выбрано."));
        return;
    }

#ifdef Q_OS_ANDROID
    const auto &device = m_devices.at(row);
    const QByteArray packet = makeTestPacket();

    QJniEnvironment env;
    jbyteArray bytes = env->NewByteArray(packet.size());
    env->SetByteArrayRegion(bytes, 0, packet.size(), reinterpret_cast<const jbyte *>(packet.constData()));

    const int sent = QJniObject::callStaticMethod<jint>(
        "org/qtproject/example/usbandroid/UsbSerialBridge",
        "write",
        "(Landroid/content/Context;Ljava/lang/String;[BI)I",
        QNativeInterface::QAndroidApplication::context().object(),
        QJniObject::fromString(device.deviceName).object<jstring>(),
        bytes,
        9600);
    env->DeleteLocalRef(bytes);

    const QString packetHex = QString::fromLatin1(packet.toHex(' ').toUpper());
    setStatus(sent == packet.size()
                  ? QStringLiteral("Отправлен Modbus RTU пакет %1 через usb-serial-for-android: %2 байт.").arg(packetHex).arg(sent)
                  : QStringLiteral("Ошибка отправки Modbus RTU пакета %1 через usb-serial-for-android, код: %2.").arg(packetHex).arg(sent));
#else
    setStatus(QStringLiteral("Отправка доступна только в Android сборке."));
#endif
}

bool UsbScanner::connectDevice(int row)
{
    if (row < 0 || row >= m_devices.size()) {
        m_lastError = QStringLiteral("Устройство не выбрано");
        setStatus(m_lastError);
        return false;
    }
    m_selectedDeviceName = m_devices.at(row).deviceName;
    m_lastError.clear();
    setStatus(QStringLiteral("Выбрано устройство: %1").arg(m_selectedDeviceName));
    return true;
}

bool UsbScanner::openPort(int baudRate, int stopBits, const QString &parity)
{
    if (m_selectedDeviceName.isEmpty()) {
        m_lastError = QStringLiteral("Сначала выберите USB устройство");
        setStatus(m_lastError);
        return false;
    }
    if (stopBits != 1 || parity.toLower() != QStringLiteral("none")) {
        m_lastError = QStringLiteral("Поддерживаются stop bits = 1 и parity = none");
        setStatus(m_lastError);
        return false;
    }
#ifdef Q_OS_ANDROID
    const int result = QJniObject::callStaticMethod<jint>(
        "org/qtproject/example/usbandroid/UsbSerialBridge",
        "openPort",
        "(Landroid/content/Context;Ljava/lang/String;I)I",
        QNativeInterface::QAndroidApplication::context().object(),
        QJniObject::fromString(m_selectedDeviceName).object<jstring>(),
        baudRate);
    if (result != 0) {
        m_lastError = QStringLiteral("Ошибка открытия COM порта, код: %1").arg(result);
        setStatus(m_lastError);
        return false;
    }
#else
    Q_UNUSED(baudRate)
#endif
    m_lastError.clear();
    setStatus(QStringLiteral("Порт открыт: %1, 8N1").arg(baudRate));
    return true;
}

QString UsbScanner::lastError() const { return m_lastError; }

void UsbScanner::setStatus(const QString &status)
{
    if (m_status == status) return;
    m_status = status;
    emit statusChanged();
}

QString UsbScanner::usbClassName(int c)
{
    switch (c) {
    case 0: return "Per-interface"; case 2: return "Communications"; case 3: return "HID";
    case 8: return "Mass storage"; case 9: return "Hub"; case 10: return "CDC data";
    case 255: return "Vendor specific"; default: return QStringLiteral("Class %1").arg(c);
    }
}

QString UsbScanner::endpointDirection(int d) { return d == UsbDirIn ? "IN" : "OUT"; }

QString UsbScanner::endpointType(int t)
{
    switch (t) {
    case UsbEndpointXferControl: return "control"; case UsbEndpointXferIsochronous: return "isochronous";
    case UsbEndpointXferBulk: return "bulk"; case UsbEndpointXferInterrupt: return "interrupt";
    default: return QStringLiteral("type %1").arg(t);
    }
}

quint16 UsbScanner::modbusCrc(const QByteArray &payload)
{
    quint16 crc = 0xFFFF;
    for (auto ch : payload) {
        crc ^= static_cast<quint8>(ch);
        for (int i = 0; i < 8; ++i)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

QByteArray UsbScanner::makeTestPacket()
{
    QByteArray p;
    p.append(char(0x01)); // slave id
    p.append(char(0x06)); // write single holding register
    p.append(char(0xF0));
    p.append(char(0x00));
    p.append(char(0x00));
    p.append(char(0x1A)); // decimal 26
    const quint16 crc = modbusCrc(p);
    p.append(char(crc & 0xFF));
    p.append(char((crc >> 8) & 0xFF));
    return p;
}
