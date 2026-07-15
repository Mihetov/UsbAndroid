
#include "registermodel.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>
#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif
namespace {
const char *builtInJson = R"JSON({"name":"Устройство №1","registers":[
{"address":"0xF000","mnemonic":"HREG_slaveID","access":"RWE","bytes":2,"format":"Word","min":1,"max":247,"description":"Сетевой адрес","factory":"247"},
{"address":"0xF020","mnemonic":"HREG_mode","access":"RWE","bytes":2,"format":"Int8","min":-128,"max":127,"description":"Режим протокола обмена","factory":"2"},
{"address":"0xF021","mnemonic":"HREG_baud","access":"RWE","bytes":2,"format":"Byte","min":0,"max":4,"description":"Скорость обмена","factory":"3"},
{"address":"0xF022","mnemonic":"HREG_timeout","access":"RWE","bytes":2,"format":"Byte","min":0,"max":7,"description":"Таймаут запросов от ведущего","factory":"3"},
{"address":"0xF032","mnemonic":"HREG_validlv","access":"RWE","bytes":2,"format":"Byte","min":0,"max":3,"description":"Источник сброса таймаута","factory":""},
{"address":"0xF033","mnemonic":"HREG_validvw","access":"RWE","bytes":2,"format":"Byte","min":0,"max":3,"description":"Управление отображением таймаута","factory":""},
{"address":"0xF001","mnemonic":"HREG_devID","access":"R","bytes":2,"format":"Word","min":0,"max":255,"description":"ID устройства","factory":""},
{"address":"0xF003","mnemonic":"HREG_hardsr","access":"RWE","bytes":2,"format":"Word","min":0,"max":65535,"description":"Состояние устройства","factory":""},
{"address":"0xF004","mnemonic":"HREG_obasis","access":"R","bytes":2,"format":"Word","min":0,"max":255,"description":"O-basis","factory":""},
{"address":"0xF008","mnemonic":"HREG_srelease","access":"R","bytes":16,"format":"String","min":0,"max":0,"description":"Версия (дата выпуска)","factory":""},
{"address":"0xF010","mnemonic":"HREG_sfirmwar","access":"R","bytes":32,"format":"String","min":0,"max":0,"description":"Прошивка","factory":""},
{"address":"0xF024","mnemonic":"HREG_serial","access":"RWE","bytes":4,"format":"Int32","min":0,"max":0,"description":"Серийный номер","factory":""},
{"address":"0xF800","mnemonic":"HREG_sdescript","access":"RWE","bytes":32,"format":"String","min":0,"max":0,"description":"Описание устройства","factory":""},
{"address":"0xF002","mnemonic":"HREG_colortyp","access":"RWE","bytes":2,"format":"Word","min":0,"max":7,"description":"Цветовая модель","factory":""},
{"address":"0xE000","mnemonic":"HREG_syslight","access":"RW","bytes":2,"format":"Word","min":0,"max":255,"description":"Линейная яркость","factory":"8"},
{"address":"0xE001","mnemonic":"HREG_idxlight","access":"RW","bytes":2,"format":"Word","min":0,"max":15,"description":"Индексированная яркость","factory":"0"},
{"address":"0xE002","mnemonic":"HREG_syscolor","access":"RW","bytes":2,"format":"Word","min":0,"max":3,"description":"Цвет свечения дисплея","factory":""},
{"address":"0xE010","mnemonic":"HREG_gaplight","access":"RWE","bytes":2,"format":"Byte","min":0,"max":255,"description":"калибровка яркости","factory":"1"},
{"address":"0xE011","mnemonic":"HREG_deflight","access":"RWE","bytes":2,"format":"Byte","min":0,"max":15,"description":"яркость по включению","factory":""},
{"address":"0xE012","mnemonic":"HREG_defcolor","access":"RWE","bytes":2,"format":"Byte","min":0,"max":3,"description":"цвет по включению","factory":""},
{"address":"0x0000","mnemonic":"HREG_DATA","access":"RW","bytes":32,"format":"Array","min":0,"max":255,"description":"Прямое управление сегментами","factory":""},
{"address":"0xF030","mnemonic":"HREG_dispmig","access":"RW","bytes":2,"format":"Byte","min":0,"max":255,"description":"Маска мигания","factory":""},
{"address":"0xD000","mnemonic":"HREG_vfloat","access":"RW","bytes":4,"format":"Float","min":0,"max":0,"description":"вывод Float32","factory":""},
{"address":"0xD002","mnemonic":"HREG_vint32","access":"RW","bytes":4,"format":"Int32","min":0,"max":0,"description":"вывод Int32","factory":""},
{"address":"0xD003","mnemonic":"HREG_vint16","access":"RW","bytes":2,"format":"Int16","min":0,"max":0,"description":"вывод Int16","factory":""},
{"address":"0xD004","mnemonic":"HREG_vpoint","access":"RWE","bytes":2,"format":"Word","min":0,"max":7,"description":"десятичная точка","factory":""},
{"address":"0xD020","mnemonic":"HREG_dispapxX0","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"X 0 точки аппроксимации","factory":""},
{"address":"0xD022","mnemonic":"HREG_dispapxY0","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"Y 0 точки аппроксимации","factory":""},
{"address":"0xD024","mnemonic":"HREG_dispapxX1","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"X 1 точки аппроксимации","factory":""},
{"address":"0xD026","mnemonic":"HREG_dispapxY1","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"Y 1 точки аппроксимации","factory":""},
{"address":"0xF038","mnemonic":"HREG_colorX1","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"автоцвет X1","factory":""},
{"address":"0xF03A","mnemonic":"HREG_colorX2","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"автоцвет X2","factory":""},
{"address":"0xF03C","mnemonic":"HREG_minFval","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"минимум","factory":""},
{"address":"0xF03E","mnemonic":"HREG_maxFval","access":"RWE","bytes":4,"format":"Float","min":0,"max":0,"description":"максимум","factory":""},
{"address":"0xF023","mnemonic":"HREG_tmaster","access":"RWE","bytes":2,"format":"Word","min":0,"max":65535,"description":"период опроса","factory":""},
{"address":"0xF026","mnemonic":"HREG_masterCM","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"команда","factory":""},
{"address":"0xF027","mnemonic":"HREG_masterWC","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"количество регистров","factory":""},
{"address":"0xF028","mnemonic":"HREG_masterID","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"адрес ведомого","factory":""},
{"address":"0xF029","mnemonic":"HREG_masterHR","access":"RWE","bytes":2,"format":"Word","min":0,"max":65535,"description":"адрес регистра","factory":""},
{"address":"0xD014","mnemonic":"HREG_dataofs","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"смещение данных","factory":""},
{"address":"0xD015","mnemonic":"HREG_datafmt","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"тип данных","factory":""},
{"address":"0xD016","mnemonic":"HREG_dataend","access":"RWE","bytes":2,"format":"Word","min":0,"max":255,"description":"порядок следования байт","factory":""},
{"address":"0xD100","mnemonic":"HREG_tCP56","access":"RW","bytes":7,"format":"TCP56","min":0,"max":0,"description":"Время в формате CP56","factory":""},
{"address":"0xD108","mnemonic":"HREG_tUTC","access":"RWE","bytes":2,"format":"Byte","min":0,"max":15,"description":"Часовой пояс","factory":"3"},
{"address":"0xD109","mnemonic":"HREG_tFMT","access":"RWE","bytes":2,"format":"Byte","min":0,"max":5,"description":"Формат отображения времени","factory":"0"},
{"address":"0xD10A","mnemonic":"HREG_timeBLK","access":"RWE","bytes":2,"format":"Word","min":0,"max":65535,"description":"Управление миганием разделителя","factory":""},
{"address":"0xD10B","mnemonic":"HREG_tASC","access":"RW","bytes":18,"format":"String","min":0,"max":0,"description":"Время в формате ASCII","factory":"01.01.2000 0:00"},
{"address":"0xD114","mnemonic":"HREG_timeCNT","access":"R","bytes":4,"format":"Int32","min":0,"max":4294967296,"description":"Счетчик секунд","factory":""}
]})JSON";
}

RegisterModel::RegisterModel(QObject *parent) : QAbstractListModel(parent)
{
    reloadModels();
    selectModel(0);
}

int RegisterModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_registers.size(); }

QVariant RegisterModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_registers.size()) return {};
    const auto &r = m_registers.at(idx.row());
    QString hint;
    const bool valid = isValidValue(r, &hint);
    switch (role) {
    case AddressRole: return r.address;
    case MnemonicRole: return r.mnemonic;
    case AccessRole: return r.access;
    case BytesRole: return r.bytes;
    case FormatRole: return r.format;
    case MinimumRole: return r.minimum;
    case MaximumRole: return r.maximum;
    case DescriptionRole: return r.status == QStringLiteral("device_error") ? QStringLiteral("Ошибка") : r.description;
    case FactoryValueRole: return r.factoryValue;
    case ValueRole: return r.value;
    case ReadValueRole: return r.readValue;
    case StatusRole: return valid ? r.status : QStringLiteral("invalid");
    case ErrorMsgRole: return r.errorMsg;
    case ExpandedRole: return r.expanded;
    case ChangedRole: return r.status == QStringLiteral("changed") || (r.errorMsg.isEmpty() && r.value != r.readValue);
    case WritableRole: return r.access.contains(u'W');
    case ValidRole: return valid;
    case HintRole: return hint;
    default: return {};
    }
}

bool RegisterModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_registers.size()) return false;
    auto &r = m_registers[idx.row()];
    if (role == ValueRole || role == Qt::EditRole) {
        r.value = value.toString();
        r.errorMsg.clear();
        r.status = (r.value == r.readValue) ? QStringLiteral("clean") : QStringLiteral("changed");
    } else if (role == ExpandedRole) {
        r.expanded = value.toBool();
    } else {
        return false;
    }
    emitRowChanged(idx.row());
    emit stateChanged();
    return true;
}

QHash<int, QByteArray> RegisterModel::roleNames() const
{
    return {{AddressRole,"address"},{MnemonicRole,"mnemonic"},{AccessRole,"access"},{BytesRole,"bytes"},{FormatRole,"format"},{MinimumRole,"minimum"},{MaximumRole,"maximum"},{DescriptionRole,"description"},{FactoryValueRole,"factoryValue"},{ValueRole,"value"},{ReadValueRole,"readValue"},{StatusRole,"status"},{ErrorMsgRole,"error_msg"},{ExpandedRole,"expanded"},{ChangedRole,"changed"},{WritableRole,"writable"},{ValidRole,"valid"},{HintRole,"hint"}};
}

QString RegisterModel::deviceName() const { return m_deviceName; }
bool RegisterModel::busy() const { return m_busy; }
void RegisterModel::setBusy(bool busy) { if (m_busy == busy) return; m_busy = busy; emit busyChanged(); }

bool RegisterModel::hasChanges() const
{
    for (const auto &r : m_registers) if (r.access.contains(u'W') && r.value != r.readValue) return true;
    return false;
}

bool RegisterModel::hasValidationErrors() const
{
    for (const auto &r : m_registers) if (!isValidValue(r)) return true;
    return false;
}

void RegisterModel::reloadModels()
{
    m_models = QJsonArray();
    loadBuiltInModel();
    loadExternalModels();
}

QVariantList RegisterModel::availableModels() const
{
    QVariantList out;
    for (const auto &model : m_models) out << model.toObject().value("name").toString();
    return out;
}

bool RegisterModel::selectModel(int index)
{
    if (index < 0 || index >= m_models.size()) return false;
    loadModelObject(m_models.at(index).toObject());
    return true;
}

void RegisterModel::setValue(int row, const QString &value) { setData(index(row), value, ValueRole); }
void RegisterModel::toggleExpanded(int row) { if (row >= 0 && row < m_registers.size()) setData(index(row), !m_registers.at(row).expanded, ExpandedRole); }

QVariantList RegisterModel::allReadableRequests() const
{
    QVariantList out;
    for (const auto &r : m_registers) if (r.access.contains(u'R')) out << requestFor(r, r.value);
    return out;
}

QVariantList RegisterModel::changedWriteRequests() const
{
    QVariantList out;
    for (const auto &r : m_registers) if (r.access.contains(u'W') && r.value != r.readValue && isValidValue(r)) out << requestFor(r, r.value);
    return out;
}

QVariantList RegisterModel::allWriteRequests() const
{
    QVariantList out;
    for (const auto &r : m_registers) if (r.access.contains(u'W') && isValidValue(r)) out << requestFor(r, r.value);
    return out;
}

QVariantList RegisterModel::allFactoryWriteRequests() const
{
    QVariantList out;
    for (const auto &r : m_registers) if (r.access.contains(u'W') && !r.factoryValue.isEmpty()) out << requestFor(r, r.factoryValue);
    return out;
}

void RegisterModel::applyFactoryDefaults()
{
    for (int i = 0; i < m_registers.size(); ++i) {
        auto &r = m_registers[i];
        if (!r.access.contains(u'W') || r.factoryValue.isEmpty()) continue;
        r.value = r.factoryValue;
        r.status = (r.value == r.readValue) ? QStringLiteral("clean") : QStringLiteral("changed");
        r.errorMsg.clear();
        emitRowChanged(i);
    }
    emit stateChanged();
}

void RegisterModel::applyReadResult(const QString &address, const QString &value, bool ok, const QString &errorMsg)
{
    const int row = rowByAddress(address);
    if (row < 0) return;
    auto &r = m_registers[row];
    if (ok) {
        r.value = value;
        r.readValue = value;
        r.status = QStringLiteral("clean");
        r.errorMsg.clear();
    } else {
        r.status = QStringLiteral("device_error");
        r.errorMsg = errorMsg;
    }
    emitRowChanged(row);
    emit stateChanged();
}

void RegisterModel::applyWriteResult(const QString &address, bool ok, const QString &errorMsg)
{
    const int row = rowByAddress(address);
    if (row < 0) return;
    auto &r = m_registers[row];
    if (ok) {
        r.readValue = r.value;
        r.status = QStringLiteral("clean");
        r.errorMsg.clear();
    } else {
        r.status = QStringLiteral("device_error");
        r.errorMsg = errorMsg;
    }
    emitRowChanged(row);
    emit stateChanged();
}

void RegisterModel::markAllReading()
{
    for (auto &r : m_registers) if (r.access.contains(u'R')) { r.status = QStringLiteral("reading"); r.errorMsg.clear(); }
    if (!m_registers.isEmpty()) emit dataChanged(index(0), index(m_registers.size() - 1));
    emit stateChanged();
}

void RegisterModel::markWriting(const QVariantList &requests)
{
    for (const auto &item : requests) {
        const int row = rowByAddress(item.toMap().value("address").toString());
        if (row >= 0) { m_registers[row].status = QStringLiteral("writing"); emitRowChanged(row); }
    }
    emit stateChanged();
}

void RegisterModel::loadBuiltInModel()
{
    auto doc = QJsonDocument::fromJson(QByteArray(builtInJson));
    if (doc.isObject()) m_models.append(doc.object());
}

void RegisterModel::loadExternalModels()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/device-models";
    QDir().mkpath(dirPath);
    QDir dir(dirPath);
    for (const auto &file : dir.entryList({"*.json"}, QDir::Files)) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly)) continue;
        auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) m_models.append(doc.object());
    }
}

void RegisterModel::loadModelObject(const QJsonObject &object)
{
    beginResetModel();
    m_registers.clear();
    m_deviceName = object.value("name").toString("Device");
    for (const auto &value : object.value("registers").toArray()) {
        const auto obj = value.toObject();
        RegisterEntry r;
        r.address = obj.value("address").toString();
        r.mnemonic = obj.value("mnemonic").toString();
        r.access = obj.value("access").toString();
        r.bytes = obj.value("bytes").toInt();
        r.format = obj.value("format").toString();
        r.minimum = obj.value("min").toDouble();
        r.maximum = obj.value("max").toDouble();
        r.description = obj.value("description").toString();
        r.factoryValue = obj.value("factory").toString();
        r.value = r.factoryValue.isEmpty() ? QStringLiteral("0") : r.factoryValue;
        r.readValue = r.value;
        m_registers << r;
    }
    endResetModel();
    emit deviceNameChanged();
    emit stateChanged();
}

QVariantMap RegisterModel::requestFor(const RegisterEntry &r, const QString &value) const
{
    return {{"address", r.address}, {"mnemonic", r.mnemonic}, {"bytes", r.bytes}, {"format", r.format}, {"value", value}};
}

int RegisterModel::rowByAddress(const QString &address) const
{
    for (int i = 0; i < m_registers.size(); ++i) if (m_registers.at(i).address.compare(address, Qt::CaseInsensitive) == 0) return i;
    return -1;
}

bool RegisterModel::isValidValue(const RegisterEntry &r, QString *hint) const
{
    if (r.status == QStringLiteral("device_error")) { if (hint) *hint = r.errorMsg; return true; }
    if (!r.access.contains(u'W') && r.value != r.readValue) { if (hint) *hint = QStringLiteral("Регистр только для чтения"); return false; }
    if (r.format == "String" || r.format == "Array" || r.format == "TCP56") { if (hint) *hint = QStringLiteral("Формат %1, до %2 байт").arg(r.format).arg(r.bytes); return true; }
    bool ok = false;
    const double d = r.value.toDouble(&ok);
    if (!ok) { if (hint) *hint = QStringLiteral("Нужно число: %1 [%2..%3]").arg(r.format).arg(r.minimum).arg(r.maximum); return false; }
    if (isIntegerFormat(r.format) && d != qint64(d)) { if (hint) *hint = QStringLiteral("Нужно целое число: %1 [%2..%3]").arg(r.format).arg(r.minimum).arg(r.maximum); return false; }
    if ((r.minimum != 0 || r.maximum != 0) && (d < r.minimum || d > r.maximum)) { if (hint) *hint = QStringLiteral("Диапазон %1: %2..%3").arg(r.format).arg(r.minimum).arg(r.maximum); return false; }
    if (hint) *hint = QStringLiteral("Формат %1, диапазон %2..%3").arg(r.format).arg(r.minimum).arg(r.maximum);
    return true;
}

bool RegisterModel::isIntegerFormat(const QString &format) { return format.contains("Int") || format == "Word" || format == "Byte"; }
void RegisterModel::emitRowChanged(int row) { emit dataChanged(index(row), index(row)); }
QVariantList RegisterModel::singleReadRequest(int row)
{
    QVariantList out;
    if (row < 0 || row >= m_registers.size()) return out;

    auto &r = m_registers[row];
    if (r.access.contains(u'R')) {
        // Меняем статус на "чтение...", чтобы интерфейс показал спиннер/загрузку
        r.status = QStringLiteral("reading");
        r.errorMsg.clear();
        emitRowChanged(row);
        emit stateChanged();

        // Формируем запрос на чтение конкретно этого регистра
        out << requestFor(r, r.value);
    }
    return out;
}
void RegisterModel::saveExternalModel(const QString &fileName, const QString &jsonContent)
{
    // Оставляем для ручного сохранения, если пригодится
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/device-models";
    QDir().mkpath(dirPath);
    QFile file(dirPath + "/" + fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << jsonContent;
        file.close();
        reloadModels();
        emit deviceNameChanged();
    }
}

// Новый метод для облачного скачивания через Java
bool RegisterModel::downloadModelFromCloud(const QString &fileUrl, const QString &fileName)
{
#if defined(Q_OS_ANDROID)
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/device-models";
    QDir().mkpath(dirPath);
    QString savePath = dirPath + "/" + fileName;

    // Вызываем статикметод Java через QJniObject
    // Путь к классу: org/qtproject/example/appUsbAndroid/DeviceDownloader
    QJniObject jUrl = QJniObject::fromString(fileUrl);
    QJniObject jPath = QJniObject::fromString(savePath);

    bool success = QJniObject::callStaticMethod<jboolean>(
        "org/qtproject/example/appUsbAndroid/DeviceDownloader",
        "downloadAndSave",
        "(Ljava/lang/String;Ljava/lang/String;)Z",
        jUrl.object<jstring>(),
        jPath.object<jstring>()
        );

    if (success) {
        // Перечитываем модели, чтобы устройство сразу появилось в UI
        reloadModels();
        emit deviceNameChanged();
    }
    return success;
#else
    // Заглушка для отладки на ПК (если захочешь протестировать под Windows/Linux)
    Q_UNUSED(fileUrl);
    Q_UNUSED(fileName);
    return false;
#endif
}