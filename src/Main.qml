import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 430
    height: 800
    visible: true
    title: qsTr("Конфигуратор Modbus")

    property int page: 0
    property int slaveId: 1

    Dialog { id: errorDialog; title: "Ошибка"; modal: true; standardButtons: Dialog.Ok; property alias message: msg.text; Label { id: msg; width: 320; wrapMode: Text.WordWrap } }
    Dialog { id: reportDialog; title: "Результат записи"; modal: true; standardButtons: Dialog.Ok; property alias message: report.text; Label { id: report; width: 340; wrapMode: Text.WordWrap } onClosed: deviceRegisterModel.readAll() }

    StackLayout {
        anchors.fill: parent
        currentIndex: page

        ColumnLayout {
            anchors.margins: 12
            Label { text: "Выбор USB устройства"; font.pixelSize: 22; font.bold: true }
            Label { Layout.fillWidth: true; text: usbScanner.status; wrapMode: Text.WordWrap }
            ListView {
                id: deviceList; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: usbScanner
                delegate: Frame { width: deviceList.width; ColumnLayout { anchors.fill: parent
                    Label { text: model.productName; font.bold: true; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    Label { text: model.devicePath + "  VID " + model.vendorId + " PID " + model.productId; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    Button { text: "Подключиться"; Layout.fillWidth: true; onClicked: { if (usbScanner.connectDevice(index)) page = 1; else { errorDialog.message = usbScanner.lastError(); errorDialog.open(); } } }
                } }
            }
            Button { text: "Сканировать USB"; Layout.fillWidth: true; onClicked: usbScanner.scan() }
        }

        ColumnLayout {
            anchors.margins: 12
            Label { text: "Параметры COM порта"; font.pixelSize: 22; font.bold: true }
            SpinBox { id: baud; from: 1200; to: 921600; value: 115200; editable: true; Layout.fillWidth: true }
            ComboBox { id: parity; model: ["none"]; Layout.fillWidth: true }
            SpinBox { id: stopBits; from: 1; to: 2; value: 1; Layout.fillWidth: true }
            Button { text: "Открыть порт"; Layout.fillWidth: true; onClicked: { if (usbScanner.openPort(baud.value, stopBits.value, parity.currentText)) { deviceRegisterModel.readAll(); page = 2; } else { errorDialog.message = usbScanner.lastError(); errorDialog.open(); } } }
            Button { text: "Назад"; Layout.fillWidth: true; onClicked: page = 0 }
            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            spacing: 0
            ToolBar { Layout.fillWidth: true; RowLayout { anchors.fill: parent
                Label { text: deviceRegisterModel.deviceName; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                Button { text: "Чтение"; onClicked: deviceRegisterModel.readAll() }
                ToolButton { text: "⋮"; onClicked: tools.open(); Menu { id: tools; MenuItem { text: "Записать все"; onTriggered: deviceRegisterModel.writeAll() } MenuItem { text: "Заводской режим"; onTriggered: deviceRegisterModel.factoryReset() } } }
            } }
            RowLayout { Layout.fillWidth: true; Layout.margins: 8
                Label { text: "Модель" }
                ComboBox { id: modelPicker; Layout.fillWidth: true; model: deviceRegisterModel.availableModels(); onActivated: { deviceRegisterModel.selectModel(currentIndex); deviceRegisterModel.readAll(); } }
            }
            RowLayout { Layout.fillWidth: true; Layout.margins: 8
                Label { text: "ID Modbus" }
                SpinBox { from: 0; to: 247; value: slaveId; enabled: !ff.checked && !fe.checked; onValueChanged: slaveId = value; Layout.fillWidth: true }
                CheckBox { id: ff; text: "FF"; onCheckedChanged: if (checked) fe.checked = false }
                CheckBox { id: fe; text: "FE"; onCheckedChanged: if (checked) ff.checked = false }
            }
            ListView { id: regs; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: deviceRegisterModel
                delegate: Rectangle { width: regs.width; implicitHeight: col.implicitHeight + 12; color: !model.valid ? "#ffd6d6" : model.changed ? "#d9f7d9" : "#eeeeee"; border.color: "#cccccc"
                    ColumnLayout { id: col; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 6
                        RowLayout { Layout.fillWidth: true
                            ToolButton { text: model.expanded ? "⌄" : "›"; onClicked: deviceRegisterModel.toggleExpanded(index) }
                            Label { text: model.description; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            TextField { text: model.value; enabled: model.writable; Layout.preferredWidth: 120; placeholderText: model.format; onEditingFinished: deviceRegisterModel.setValue(index, text) }
                        }
                        Label { visible: !model.valid; text: model.hint; color: "#a00000"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        Label { visible: model.expanded; text: "Адрес: " + model.address + "\nМнемоника: " + model.mnemonic + "\nТип: " + model.access + ", байт: " + model.bytes + "\nФормат: " + model.format + ", мин: " + model.minimum + ", макс: " + model.maximum + (model.error ? "\nОшибка Modbus: " + model.error : ""); Layout.fillWidth: true; wrapMode: Text.WordWrap; font.family: "monospace" }
                    }
                }
            }
            ToolBar { Layout.fillWidth: true; Button { anchors.fill: parent; text: "Записать"; enabled: deviceRegisterModel.hasChanges && !deviceRegisterModel.hasValidationErrors; onClicked: { reportDialog.message = deviceRegisterModel.writeChanged(); reportDialog.open(); } } }
        }
    }
    Component.onCompleted: usbScanner.scan()
}
