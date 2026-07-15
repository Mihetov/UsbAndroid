#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "usbscanner.h"
#include "backend.h"
#include "registermodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    UsbScanner usbScanner;
    RegisterModel registerModel;
    Backend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("usbScanner"), &usbScanner);
    engine.rootContext()->setContextProperty(QStringLiteral("registerModel"), &registerModel);
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    QObject::connect(&backend, &Backend::readStarted, &registerModel, &RegisterModel::markAllReading);
    QObject::connect(&backend, &Backend::writeStarted, &registerModel, &RegisterModel::markWriting);
    QObject::connect(&backend, &Backend::registerRead, &registerModel, &RegisterModel::applyReadResult);
    QObject::connect(&backend, &Backend::registerWritten, &registerModel, &RegisterModel::applyWriteResult);
    QObject::connect(&backend, &Backend::busyChanged, &registerModel, [&]() { registerModel.setBusy(backend.busy()); });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("UsbAndroid", "Main");

    return app.exec();
}
