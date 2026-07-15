#include "deviceregistermodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>

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

DeviceRegisterModel::DeviceRegisterModel(QObject *parent) : QAbstractListModel(parent) { loadModels(); selectModel(0); }
int DeviceRegisterModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_registers.size(); }
QVariant DeviceRegisterModel::data(const QModelIndex &idx, int role) const { if (!idx.isValid() || idx.row()<0 || idx.row()>=m_registers.size()) return {}; const auto &r=m_registers.at(idx.row()); QString hint; const bool valid=isValidValue(r,&hint); switch(role){case AddressRole:return r.address;case MnemonicRole:return r.mnemonic;case AccessRole:return r.access;case BytesRole:return r.bytes;case FormatRole:return r.format;case MinimumRole:return r.minimum;case MaximumRole:return r.maximum;case DescriptionRole:return r.error.isEmpty()?r.description:QStringLiteral("Ошибка");case FactoryValueRole:return r.factoryValue;case ValueRole:return r.value;case ReadValueRole:return r.readValue;case ErrorRole:return r.error;case ExpandedRole:return r.expanded;case ChangedRole:return r.error.isEmpty()&&r.value!=r.readValue;case WritableRole:return r.access.contains('W');case ValidRole:return valid;case HintRole:return hint;default:return {};}}
bool DeviceRegisterModel::setData(const QModelIndex &idx,const QVariant &v,int role){ if(!idx.isValid()||idx.row()<0||idx.row()>=m_registers.size()) return false; auto &r=m_registers[idx.row()]; if(role==ValueRole||role==Qt::EditRole) r.value=v.toString(); else if(role==ExpandedRole) r.expanded=v.toBool(); else return false; emit dataChanged(idx,idx); refreshState(); return true; }
QHash<int,QByteArray> DeviceRegisterModel::roleNames() const { return {{AddressRole,"address"},{MnemonicRole,"mnemonic"},{AccessRole,"access"},{BytesRole,"bytes"},{FormatRole,"format"},{MinimumRole,"minimum"},{MaximumRole,"maximum"},{DescriptionRole,"description"},{FactoryValueRole,"factoryValue"},{ValueRole,"value"},{ReadValueRole,"readValue"},{ErrorRole,"error"},{ExpandedRole,"expanded"},{ChangedRole,"changed"},{WritableRole,"writable"},{ValidRole,"valid"},{HintRole,"hint"}}; }
QString DeviceRegisterModel::deviceName() const{return m_deviceName;} bool DeviceRegisterModel::hasChanges() const{return m_hasChanges;} bool DeviceRegisterModel::hasValidationErrors() const{return m_hasValidationErrors;}
void DeviceRegisterModel::loadModels(){ m_models=QJsonArray(); loadBuiltInModel(); loadExternalModels(); }
QVariantList DeviceRegisterModel::availableModels() const { QVariantList out; for(const auto &v:m_models) out<<v.toObject().value("name").toString(); return out; }
bool DeviceRegisterModel::selectModel(int i){ if(i<0||i>=m_models.size()) return false; loadModelObject(m_models.at(i).toObject()); return true; }
void DeviceRegisterModel::setValue(int row,const QString &v){ setData(index(row),v,ValueRole); }
void DeviceRegisterModel::toggleExpanded(int row){ if(row>=0&&row<m_registers.size()) setData(index(row),!m_registers.at(row).expanded,ExpandedRole); }
void DeviceRegisterModel::readAll(){ for(auto &r:m_registers){ r.error.clear(); r.readValue=r.factoryValue.isEmpty()?QStringLiteral("0"):r.factoryValue; r.value=r.readValue; } emit dataChanged(index(0),index(m_registers.size()-1)); refreshState(); }
QString DeviceRegisterModel::writeChanged(){ QStringList written, errors; for(auto &r:m_registers){ QString hint; if(r.value!=r.readValue){ if(!r.access.contains('W')||!isValidValue(r,&hint)) errors<<QStringLiteral("%1 %2: %3").arg(r.address,r.mnemonic,hint); else { r.readValue=r.value; written<<QStringLiteral("%1 %2").arg(r.address,r.mnemonic); } } } emit dataChanged(index(0),index(m_registers.size()-1)); refreshState(); return QStringLiteral("Записано:\n%1\n\nОшибки:\n%2").arg(written.isEmpty()?QStringLiteral("нет"):written.join('\n'), errors.isEmpty()?QStringLiteral("нет"):errors.join('\n')); }
void DeviceRegisterModel::writeAll(){ for(auto &r:m_registers) if(r.access.contains('W') && isValidValue(r)) r.readValue=r.value; emit dataChanged(index(0),index(m_registers.size()-1)); refreshState(); }
void DeviceRegisterModel::factoryReset(){ for(auto &r:m_registers) if(r.access.contains('E')) r.value=r.factoryValue; refreshState(); emit dataChanged(index(0),index(m_registers.size()-1)); }
void DeviceRegisterModel::loadBuiltInModel(){ auto doc=QJsonDocument::fromJson(QByteArray(builtInJson)); if(doc.isObject()) m_models.append(doc.object()); }
void DeviceRegisterModel::loadExternalModels(){ const QString dirPath=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/device-models"; QDir().mkpath(dirPath); QDir dir(dirPath); for(const auto &file:dir.entryList({"*.json"},QDir::Files)){ QFile f(dir.filePath(file)); if(f.open(QIODevice::ReadOnly)){ auto doc=QJsonDocument::fromJson(f.readAll()); if(doc.isObject()) m_models.append(doc.object()); } } }
void DeviceRegisterModel::loadModelObject(const QJsonObject &o){ beginResetModel(); m_registers.clear(); m_deviceName=o.value("name").toString("Устройство"); for(const auto &v:o.value("registers").toArray()){ auto obj=v.toObject(); RegisterItem r; r.address=obj.value("address").toString(); r.mnemonic=obj.value("mnemonic").toString(); r.access=obj.value("access").toString(); r.bytes=obj.value("bytes").toInt(); r.format=obj.value("format").toString(); r.minimum=obj.value("min").toDouble(); r.maximum=obj.value("max").toDouble(); r.description=obj.value("description").toString(); r.factoryValue=obj.value("factory").toString(); r.readValue=r.factoryValue.isEmpty()?QStringLiteral("0"):r.factoryValue; r.value=r.readValue; m_registers<<r; } endResetModel(); emit deviceNameChanged(); refreshState(); }
void DeviceRegisterModel::refreshState(){ bool ch=false, er=false; for(const auto &r:m_registers){ ch|=(r.value!=r.readValue); er|=!isValidValue(r); } if(ch!=m_hasChanges){m_hasChanges=ch; emit hasChangesChanged();} if(er!=m_hasValidationErrors){m_hasValidationErrors=er; emit hasValidationErrorsChanged();} }
bool DeviceRegisterModel::isValidValue(const RegisterItem &r, QString *hint) const{ if(!r.error.isEmpty()){ if(hint)*hint=r.error; return false;} if(!r.access.contains('W')&&r.value!=r.readValue){ if(hint)*hint=QStringLiteral("Регистр только для чтения"); return false;} if(r.format=="String"||r.format=="Array"||r.format=="TCP56"){ if(hint)*hint=QStringLiteral("Формат %1, до %2 байт").arg(r.format).arg(r.bytes); return true;} bool ok=false; double d=r.value.toDouble(&ok); if(!ok){ if(hint)*hint=QStringLiteral("Нужно число: %1 [%2..%3]").arg(r.format).arg(r.minimum).arg(r.maximum); return false;} if(isIntegerFormat(r.format)&&d!=qint64(d)){ if(hint)*hint=QStringLiteral("Нужно целое число: %1 [%2..%3]").arg(r.format).arg(r.minimum).arg(r.maximum); return false;} if((r.minimum!=0||r.maximum!=0)&&(d<r.minimum||d>r.maximum)){ if(hint)*hint=QStringLiteral("Диапазон %1: %2..%3").arg(r.format).arg(r.minimum).arg(r.maximum); return false;} if(hint)*hint=QStringLiteral("Формат %1, диапазон %2..%3").arg(r.format).arg(r.minimum).arg(r.maximum); return true; }
bool DeviceRegisterModel::isIntegerFormat(const QString &f){ return f.contains("Int")||f=="Word"||f=="Byte"; }
bool DeviceRegisterModel::isNumericFormat(const QString &f){ return isIntegerFormat(f)||f=="Float"; }
