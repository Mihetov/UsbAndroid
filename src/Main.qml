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
    property string selectedDevicePath: ""
    property bool broadcastMode: ff.checked || fe.checked
    property int effectiveSlaveId: ff.checked ? 255 : (fe.checked ? 254 : slaveId)
    function downloadDeviceModel(url, fileName) {
        var success = registerModel.downloadModelFromCloud(url, fileName);

        if (success) {
            reportDialog.message = "Устройство успешно скачано и добавлено в базу!";
            reportDialog.open();
        } else {
            errorDialog.message = "Ошибка при скачивании из облака. Проверьте интернет-соединение.";
            errorDialog.open();
        }
    }

    Dialog { id: errorDialog; title: "Ошибка"; modal: true; standardButtons: Dialog.Ok; property alias message: msg.text; Label { id: msg; width: 320; wrapMode: Text.WordWrap } }
    Dialog { id: reportDialog; title: "Результат операции"; modal: true; standardButtons: Dialog.Ok; property alias message: report.text; width: 380; Label { id: report; width: parent ? parent.width : 0; wrapMode: Text.WordWrap } }

    Connections {
        target: backend
        function onPortOpened() { registerModel.markAllReading(); backend.readAll(effectiveSlaveId, registerModel.allReadableRequests()); page = 2 }
        function onPortOpenFailed(errorText) { errorDialog.message = errorText; errorDialog.open() }
        function onOperationFinished(summary, ok) { reportDialog.message = summary; if (!ok) reportDialog.open() }
    }

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
                    Button { text: "Выбрать"; Layout.fillWidth: true; onClicked: { selectedDevicePath = model.devicePath; page = 1 } }
                } }
            }
            Button { text: "Сканировать USB"; Layout.fillWidth: true; enabled: !backend.busy; onClicked: usbScanner.scan() }
        }

        ColumnLayout {
            anchors.margins: 12
            Label { text: "Параметры COM порта"; font.pixelSize: 22; font.bold: true }
            Label { text: selectedDevicePath; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            SpinBox { id: baud; from: 1200; to: 921600; value: 115200; editable: true; Layout.fillWidth: true }
            ComboBox { id: parity; model: ["none"]; Layout.fillWidth: true }
            SpinBox { id: stopBits; from: 1; to: 2; value: 1; Layout.fillWidth: true }
            Button { text: backend.busy ? "Открытие..." : "Открыть порт"; enabled: !backend.busy; Layout.fillWidth: true; onClicked: backend.connectDevice(selectedDevicePath, baud.value, stopBits.value, parity.currentText) }
            Button { text: "Назад"; enabled: !backend.busy; Layout.fillWidth: true; onClicked: page = 0 }
            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            spacing: 0
            ToolBar { Layout.fillWidth: true; RowLayout { anchors.fill: parent
                Label { text: registerModel.deviceName; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                Button { text: backend.busy ? "Чтение..." : "Чтение"; enabled: !backend.busy && !broadcastMode; onClicked: backend.readAll(effectiveSlaveId, registerModel.allReadableRequests()) }
                ToolButton { text: "⋮"; enabled: !backend.busy; onClicked: tools.open(); Menu { id: tools; MenuItem { text: "Записать все"; onTriggered: backend.writeSelected(effectiveSlaveId, registerModel.allWriteRequests()) } MenuItem { text: "Заводской режим"; onTriggered: { registerModel.applyFactoryDefaults(); backend.writeFactoryDefaults(effectiveSlaveId, registerModel.allFactoryWriteRequests()) } } MenuItem { text: "Обновить из облака"; onTriggered: {downloadDeviceModel("https://raw.githubusercontent.com/Mihetov/UsbAndroid/main/models/sdm120.json", "sdm120.json")}
                        } } }
            } }
            RowLayout { Layout.fillWidth: true; Layout.margins: 8
                Label { text: "Модель" }
                ComboBox { id: modelPicker; Layout.fillWidth: true; model: registerModel.availableModels(); enabled: !backend.busy; onActivated: { registerModel.selectModel(currentIndex); if (!broadcastMode) backend.readAll(effectiveSlaveId, registerModel.allReadableRequests()); } }
            }
            RowLayout { Layout.fillWidth: true; Layout.margins: 8
                Label { text: "ID Modbus" }
                SpinBox { from: 0; to: 247; value: slaveId; enabled: !broadcastMode && !backend.busy; onValueChanged: slaveId = value; Layout.fillWidth: true }
                CheckBox { id: ff; text: "FF"; enabled: !backend.busy; onCheckedChanged: if (checked) fe.checked = false }
                CheckBox { id: fe; text: "FE"; enabled: !backend.busy; onCheckedChanged: if (checked) ff.checked = false }
            }
            ListView { id: regs; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: registerModel
                delegate: Rectangle { width: regs.width; implicitHeight: col.implicitHeight + 12; color: model.status === "device_error" || model.status === "invalid" ? "#ffd6d6" : model.changed ? "#d9f7d9" : "#eeeeee"; border.color: "#cccccc"
                    ColumnLayout { id: col; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 6
                        RowLayout { Layout.fillWidth: true
                            ToolButton { text: model.expanded ? "⌄" : "›"; onClicked: registerModel.toggleExpanded(index) }
                            Label { text: model.description; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            TextField { text: model.value; enabled: model.writable && !backend.busy; Layout.preferredWidth: 120; placeholderText: model.format; onEditingFinished: registerModel.setValue(index, text) }
                        }
                        Label { visible: !model.valid || model.error_msg.length > 0; text: model.error_msg.length > 0 ? model.error_msg : model.hint; color: "#a00000"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        ColumnLayout { visible: model.expanded ; Layout.fillWidth: true ; spacing: 8;
                            Label { visible: model.expanded; text: "Адрес: " + model.address + "\nМнемоника: " + model.mnemonic + "\nТип: " + model.access + ", байт: " + model.bytes + "\nФормат: " + model.format + ", мин: " + model.minimum + ", макс: " + model.maximum + (model.error_msg ? "\nОшибка Modbus: " + model.error_msg : ""); Layout.fillWidth: true; wrapMode: Text.WordWrap; font.family: "monospace" }
                            Button { text: "Прочитать" ; Layout.fillWidth: true ; enabled: model.access.indexOf('R') !== -1 && !backend.busy && !broadcastMode; onClicked: {var singleReq = registerModel.singleReadRequest(index); if (singleReq.length > 0) backend.readAll(effectiveSlaveId, singleReq)}
                            }
                        }
                    }
                }
            }
            ToolBar { Layout.fillWidth: true; Button { anchors.fill: parent; text: backend.busy ? "Запись..." : "Записать"; enabled: registerModel.hasChanges && !registerModel.hasValidationErrors && !backend.busy; onClicked: backend.writeSelected(effectiveSlaveId, registerModel.changedWriteRequests()) } }
        }
    }
    Component.onCompleted: usbScanner.scan()
}
