#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QObject>
#include <kosio/core.hpp>
#include "vosfs/ui/client.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    auto signal_brige = vosfs::ui::SignalBrige{};
    auto vosfs_client = vosfs::ui::VosfsClient("127.0.0.1", 9000, signal_brige);
    auto* vosfs_thread = new QThread;
    vosfs_thread->setParent(&app);

    vosfs_client.moveToThread(vosfs_thread);
    engine.rootContext()->setContextProperty("SignalBrige", &signal_brige);
    engine.rootContext()->setContextProperty("VosfsClient", &vosfs_client);

    QObject::connect(vosfs_thread, &QThread::started, &vosfs_client, &vosfs::ui::VosfsClient::run);
    // QObject::connect(vosfs_thread, &QThread::finished, &auth_client, &vosfs::ui::VosfsClient::deleteLater);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&](){
        vosfs_client.shutdown();
        vosfs_thread->quit();
        vosfs_thread->wait();
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    vosfs_thread->start();

    engine.loadFromModule("main", "Main");
    return QGuiApplication::exec();
}