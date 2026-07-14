#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "usbscanner.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    UsbScanner usbScanner;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("usbScanner"), &usbScanner);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("UsbAndroid", "Main");

    return app.exec();
}
