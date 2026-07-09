#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>

#include "QrGenerator.h"
#include "QrScanner.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    using namespace Qt::StringLiterals;

    qmlRegisterType<QrGenerator>("QrApp", 1, 0, "QrGenerator");
    qmlRegisterType<QrScanner>("QrApp", 1, 0, "QrScanner");

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/qt/qml/QrApp/qml/Main.qml"_s);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
