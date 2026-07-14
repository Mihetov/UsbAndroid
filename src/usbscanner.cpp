#include "usbscanner.h"

#include <QCoreApplication>
#include <QJniEnvironment>
#include <QNativeInterface>

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
    auto &device = m_devices[row];
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject usbManager = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("usb").object<jstring>());
    if (!usbManager.callMethod<jboolean>("hasPermission", "(Landroid/hardware/usb/UsbDevice;)Z", device.javaDevice.object())) {
        setStatus(QStringLiteral("Нет USB permission для %1. Разрешите доступ и повторите.").arg(device.deviceName));
        return;
    }

    QJniObject connection = usbManager.callObjectMethod("openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;", device.javaDevice.object());
    if (!connection.isValid()) {
        setStatus(QStringLiteral("Не удалось открыть устройство %1.").arg(device.deviceName));
        return;
    }

    QJniObject outEndpoint;
    QJniObject outInterface;
    for (int i = 0; i < device.javaDevice.callMethod<jint>("getInterfaceCount", "()I") && !outEndpoint.isValid(); ++i) {
        QJniObject iface = device.javaDevice.callObjectMethod("getInterface", "(I)Landroid/hardware/usb/UsbInterface;", i);
        for (int e = 0; e < iface.callMethod<jint>("getEndpointCount", "()I"); ++e) {
            QJniObject ep = iface.callObjectMethod("getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;", e);
            if (ep.callMethod<jint>("getDirection", "()I") == UsbDirOut && ep.callMethod<jint>("getType", "()I") == UsbEndpointXferBulk) {
                outEndpoint = ep;
                outInterface = iface;
                break;
            }
        }
    }
    if (!outEndpoint.isValid() || !connection.callMethod<jboolean>("claimInterface", "(Landroid/hardware/usb/UsbInterface;Z)Z", outInterface.object(), true)) {
        setStatus(QStringLiteral("Bulk OUT endpoint не найден или interface не занят."));
        connection.callMethod<void>("close", "()V");
        return;
    }

    const QByteArray packet = makeTestPacket();
    QJniEnvironment env;
    jbyteArray bytes = env->NewByteArray(packet.size());
    env->SetByteArrayRegion(bytes, 0, packet.size(), reinterpret_cast<const jbyte *>(packet.constData()));
    const int sent = connection.callMethod<jint>("bulkTransfer", "(Landroid/hardware/usb/UsbEndpoint;[BII)I", outEndpoint.object(), bytes, packet.size(), 1000);
    env->DeleteLocalRef(bytes);
    connection.callMethod<jboolean>("releaseInterface", "(Landroid/hardware/usb/UsbInterface;)Z", outInterface.object());
    connection.callMethod<void>("close", "()V");
    setStatus(sent == packet.size() ? QStringLiteral("Отправлен тестовый Modbus RTU пакет: %1 байт.").arg(sent)
                                    : QStringLiteral("Ошибка отправки Modbus RTU пакета, результат: %1.").arg(sent));
#else
    setStatus(QStringLiteral("Отправка доступна только в Android сборке."));
#endif
}

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
