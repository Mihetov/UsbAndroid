#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "usbscanner.h"
#include "deviceregistermodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    UsbScanner usbScanner;
    DeviceRegisterModel deviceRegisterModel;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("usbScanner"), &usbScanner);
    engine.rootContext()->setContextProperty(QStringLiteral("deviceRegisterModel"), &deviceRegisterModel);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("UsbAndroid", "Main");

    return app.exec();
}
