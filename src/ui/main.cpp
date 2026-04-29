#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QObject>
#include <kosio/core.hpp>
#include "vosfs/ui/auth_client.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    auto auth_client = vosfs::ui::AuthClient("127.0.0.1", 9000);
    auto* auth_thread = new QThread;
    auth_thread->setParent(&app);

    auth_client.moveToThread(auth_thread);
    engine.rootContext()->setContextProperty("AuthClient", &auth_client);

    QObject::connect(auth_thread, &QThread::started, &auth_client, &vosfs::ui::AuthClient::run);
    // QObject::connect(auth_thread, &QThread::finished, &auth_client, &vosfs::ui::AuthClient::deleteLater);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&](){
        auth_client.shutdown();
        auth_thread->quit();
        auth_thread->wait();
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    auth_thread->start();

    engine.loadFromModule("main", "Main");
    return QGuiApplication::exec();
}