import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 420
    height: 760
    visible: true
    title: qsTr("USB Android Modbus")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: usbScanner.status
            wrapMode: Text.WordWrap
        }

        ListView {
            id: deviceList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 12
            model: usbScanner

            delegate: Frame {
                width: deviceList.width

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    Label { text: "Device path: " + model.devicePath; font.bold: true; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: "Device class: " + model.deviceClass; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: "Vendor ID: " + model.vendorId; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: "Product ID: " + model.productId; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: "Product name: " + model.productName; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: model.details; font.family: "monospace"; wrapMode: Text.Wrap; Layout.fillWidth: true }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Отправить тестовый Modbus RTU пакет")
                        onClicked: usbScanner.sendTestPacket(index)
                    }
                }
            }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Сканировать USB")
            onClicked: usbScanner.scan()
        }
    }

    Component.onCompleted: usbScanner.scan()
}
